#include <windows.h>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <tlhelp32.h>

#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"

#include "Hooking.Patterns.h"

extern std::wstring DefaultPathW;
extern std::vector<DWORD> Keys;
extern std::ofstream logfileA;
extern std::wofstream logfileW;
extern size_t bufSize;
extern bool isRecording;
extern char* m_Message;
extern uint8_t* bDisplayHud;
extern uint8_t* bDisplayRadar;
extern void ToggleRecording();
extern std::filesystem::path GetLatestFileName();
extern void LogRecordingW2();

extern wchar_t lastMsg[400];

void DoSuspendThread(DWORD targetProcessId, DWORD targetThreadId, bool action)
{
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te))
        {
            do
            {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
                {
                    if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
                    {
                        HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if (thread != NULL)
                        {
                            if (action)
                                SuspendThread(thread);
                            else
                                ResumeThread(thread);
                            CloseHandle(thread);
                        }
                    }
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);
    }
}

void InitIV()
{
    static auto ptr = *(uint32_t**)injector::aslr_ptr(0x88B248 + 3).get();
    struct Hack1
    {
        void operator()(injector::reg_pack& regs)
        {
            if (!m_Message)
                m_Message = *(char**)ptr + 0x38;

            *(uint16_t*)regs.edx = 0;

            auto str = (wchar_t*)m_Message;
            if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~'))
            {
                if (wcscmp((wchar_t*)m_Message, (wchar_t*)lastMsg) != 0)
                {
                    if (isRecording)
                    {
                        ToggleRecording();
                        isRecording = false;
                        DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                        Sleep(500);
                        LogRecordingW2();
                        DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
                    }
                }
                else
                {
                    if (!isRecording)
                    {
                        ToggleRecording();
                        isRecording = true;
                    }
                }
                wcscpy((wchar_t*)lastMsg, (wchar_t*)m_Message);
            }

            if (isRecording)
            {
                *bDisplayHud = 0;
                *bDisplayRadar = 0;
            }
            else
            {
                *bDisplayHud = 1;
                *bDisplayRadar = 1;
            }
        }
    }; injector::MakeInline<Hack1>(injector::aslr_ptr(0x88B24F));

    static auto ptr2 = *(uint32_t**)injector::aslr_ptr(0x8AC099 + 2).get();
    struct Hack2
    {
        void operator()(injector::reg_pack& regs)
        {
            ptr2[regs.edx] = 0;

            if (isRecording)
            {
                ToggleRecording();
                isRecording = false;
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                Sleep(500);
                LogRecordingW2();
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
            }

            if (isRecording)
            {
                *bDisplayHud = 0;
                *bDisplayRadar = 0;
            }
            else
            {
                *bDisplayHud = 1;
                *bDisplayRadar = 1;
            }
        }
    }; injector::MakeInline<Hack2>(injector::aslr_ptr(0x8AC099), injector::aslr_ptr(0x8AC099 + 10));

    static auto ptr3 = *(uint32_t**)injector::aslr_ptr(0x89E46C + 2).get();
    struct Hack3
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *ptr3;

            auto x = *(uint32_t*)injector::aslr_ptr(0x11F8908).get();
            auto y = *(uint32_t*)injector::aslr_ptr(0x11F890C).get();

            if (*(uint32_t*)(regs.eax + 0x20) == x && *(uint32_t*)(regs.eax + 0x24) == y)
            {
                *(float*)(regs.eax + 0x20) *= 1.9f;
                *(float*)(regs.eax + 0x24) *= 1.9f;
            }
        }
    }; injector::MakeInline<Hack3>(injector::aslr_ptr(0x89E46C), injector::aslr_ptr(0x89E46C+6));

    auto t = std::thread([]() 
        {
            if (bDisplayHud && bDisplayRadar)
            {
                if (isRecording)
                {
                    *bDisplayHud = 0;
                    *bDisplayRadar = 0;
                }
                else
                {
                    *bDisplayHud = 1;
                    *bDisplayRadar = 1;
                }
            }
        });
    t.detach();
}