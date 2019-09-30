#include <windows.h>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

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
extern void LogRecordingW();
extern wchar_t lastMsg[400];
extern void DoSuspendThread(DWORD targetProcessId, DWORD targetThreadId, bool action);

void InitMH()
{
    struct Hack1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)0x722034 = 0;

            if (isRecording)
            {
                ToggleRecording();
                isRecording = false;
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                Sleep(500);
                LogRecordingW();
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
            }

            *bDisplayHud = 1;
        }
    };
    injector::MakeInline<Hack1>(0x489E03, 0x489E03 + 10);
    injector::MakeInline<Hack1>(0x489EAB, 0x489EAB + 10);

    struct Hack11
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x1990) = 0;

            if (regs.ebx + 0x1990 == 0x722034)
            {
                if (isRecording)
                {
                    ToggleRecording();
                    isRecording = false;
                    DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                    Sleep(500);
                    LogRecordingW();
                    DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
                }

                *bDisplayHud = 1;
            }
        }
    };
    injector::MakeInline<Hack11>(0x48A49D - 2, 0x48A49D - 2 + 10);
    injector::MakeInline<Hack11>(0x48A715 - 2, 0x48A715 - 2 + 10);

    struct Hack111
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + 0x1990) = 0;

            if (regs.ecx + 0x1990 == 0x722034)
            {
                if (isRecording)
                {
                    ToggleRecording();
                    isRecording = false;
                    DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                    Sleep(500);
                    LogRecordingW();
                    DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
                }

                *bDisplayHud = 1;
            }
        }
    };
    injector::MakeInline<Hack111>(0x48A3B2 - 2, 0x48A3B2 - 2 + 10);
    injector::MakeInline<Hack111>(0x48A552 - 2, 0x48A552 - 2 + 10);
    injector::MakeInline<Hack111>(0x57F412 - 2, 0x57F412 - 2 + 10);

    struct Hack1111
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.edx + 0x1990) = 0;

            if (regs.edx + 0x1990 == 0x722034)
            {
                if (isRecording)
                {
                    ToggleRecording();
                    isRecording = false;
                    DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                    Sleep(500);
                    LogRecordingW();
                    DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
                }

                *bDisplayHud = 1;
            }
        }
    };
    injector::MakeInline<Hack1111>(0x528E62 - 2, 0x528E62 - 2 + 10);

    struct Hack2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x1990) = 1;

            if (regs.ebx + 0x1990 == 0x722034)
            {
                auto str = (wchar_t*)m_Message;
                //if (*(uint32_t*)0x7CF0F0 != 0) //cutscene check
                {
                    if (wcscmp(str, lastMsg) != 0 && lastMsg[0] != 0)
                    {
                        if (isRecording)
                        {
                            ToggleRecording();
                            isRecording = false;
                            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                            Sleep(500);
                            LogRecordingW();
                            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);

                            ToggleRecording();
                            isRecording = true;
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
                }
                wcscpy(lastMsg, str);

                if (isRecording)
                    * bDisplayHud = 0;
                else
                    *bDisplayHud = 1;
            }
        }
    }; injector::MakeInline<Hack2>(0x48AE55 - 2, 0x48AE55 - 2 + 10);

    struct Hack3
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)0x7A1364 = 0;
            if (isRecording)
            {
                ToggleRecording();
                isRecording = false;
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                Sleep(500);
                wcscpy(lastMsg, L"EXECUTION");
                LogRecordingW();
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
            }
        }
    };
    injector::MakeInline<Hack3>(0x58FA5D, 0x58FA5D + 7);
    injector::MakeInline<Hack3>(0x590F9D, 0x590F9D + 7);

    struct Hack4
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)0x7A1364 = 1;
            if (!isRecording)
            {
                ToggleRecording();
                isRecording = true;
            }
        }
    };
    injector::MakeInline<Hack4>(0x590440, 0x590440 + 7);

    injector::WriteMemory<float>(0x48A409 + 6, *(float*)(0x48A409 + 6) * 1.5f, true);
    injector::WriteMemory<float>(0x48A413 + 6, *(float*)(0x48A413 + 6) * 1.5f, true);
}