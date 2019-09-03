#include <windows.h>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <tlhelp32.h>

#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"

#include "Hooking.Patterns.h"

#include <RestartManager.h>
#pragma comment(lib ,"Rstrtmgr.lib")

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

extern wchar_t lastMsg[4000];

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
            }
            while (Thread32Next(h, &te));
        }
        CloseHandle(h);
    }
}

void __cdecl sub_873B40(char* a1, wchar_t* a2)
{
    if (a1)
    {
        wchar_t* v3;
        for (v3 = a2; *a1; ++v3)
        {
            *v3 = 0;
            *(BYTE*)v3 = *a1++;
        }
        *v3 = 0;
    }
}

BOOL CheckForFileLock(LPCWSTR pFilePath, bool bReleaseLock = false)
{
    BOOL bResult = FALSE;

    DWORD dwSession;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = { 0 };
    DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);
    if (dwError == ERROR_SUCCESS)
    {
        dwError = RmRegisterResources(dwSession, 1, &pFilePath, 0, NULL, 0, NULL);
        if (dwError == ERROR_SUCCESS)
        {
            UINT nProcInfoNeeded = 0;
            UINT nProcInfo = 0;
            RM_PROCESS_INFO rgpi[1];
            DWORD dwReason;

            dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, rgpi, &dwReason);
            if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
            {
                if (nProcInfoNeeded > 0)
                {
                    //If current process does not have enough privileges to close one of
                    //the "offending" processes, you'll get ERROR_FAIL_NOACTION_REBOOT
                    if (bReleaseLock)
                    {
                        dwError = RmShutdown(dwSession, RmForceShutdown, NULL);
                        if (dwError == ERROR_SUCCESS)
                        {
                            bResult = TRUE;
                        }
                    }
                }
                else
                    bResult = TRUE;
            }
        }
    }

    RmEndSession(dwSession);

    SetLastError(dwError);
    return bResult;
}

bool CheckForLockedFiles()
{
    for (auto& p : std::filesystem::directory_iterator(DefaultPathW))
    {
        if (p.path().extension() == ".mp4")
        {
            if (!CheckForFileLock(p.path().wstring().c_str()))
                return false;
        }
    }
    return true;
}

size_t count = 0;
void __cdecl sub_8AA3F0(wchar_t* a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, wchar_t* a10)
{
    char src[20];
    wchar_t dst[20];

    if (a1)
    {
        char v27 = 0;
        if (a9 < 0)
            _snprintf(src, 20u, "%d", a2);
        else
            _snprintf(src, 20u, "%.*f", a9, a8);
        src[19] = 0;
        auto v10 = strlen(src);
        sub_873B40(src, dst);
        auto v11 = 0;
        auto v28 = 0;
        auto v12 = a1;
        if (*a1)
        {
            do
            {
                ++v12;
                ++v11;
            }
            while (*v12);
            v28 = v11;
        }
        auto v26 = 0;
        if (v11 >= 600)
        {
            v28 = 599;
            v11 = 599;
            v26 = 1;
        }
        auto v13 = 0;
        auto v14 = 0;
        if (v11)
        {
            auto v15 = a1 + 2;
            do
            {
                auto v16 = a1[v13];
                wchar_t v17;
                if ((v16 == 126 || v16 == -32642)
                        && ((v17 = a1[v13 + 1], v17 == 49) || v17 == -32719)
                        && (*v15 == 126 || *v15 == -32642))
                {
                    auto v30 = v13 + 3;
                    auto v29 = (int)(v15 + 3);
                    if (v10)
                    {
                        auto v18 = &a10[v14];
                        memcpy(v18, dst, 4 * (v10 >> 1));
                        auto v20 = &dst[2 * (v10 >> 1)];
                        auto v19 = &v18[2 * (v10 >> 1)];
                        for (auto i = v10 & 1; i; --i)
                        {
                            *v19 = *v20;
                            ++v20;
                            ++v19;
                        }
                        v14 += v10;
                    }
                    switch (++v27)
                    {
                    case 1:
                        _snprintf(src, 0x14u, "%d", a3);
                        break;
                    case 2:
                        _snprintf(src, 0x14u, "%d", a4);
                        break;
                    case 3:
                        _snprintf(src, 0x14u, "%d", a5);
                        break;
                    case 4:
                        _snprintf(src, 0x14u, "%d", a6);
                        break;
                    case 5:
                        _snprintf(src, 0x14u, "%d", a7);
                        break;
                    default:
                        break;
                    }
                    src[19] = 0;
                    v10 = strlen(src);
                    sub_873B40(src, dst);
                    v11 = v28;
                    v13 = v30;
                    v15 = (wchar_t*)v29;
                }
                else
                {
                    a10[v14++] = v16;
                    ++v13;
                    ++v15;
                }
            }
            while (v13 < v11);
            if (v14 >= 0x258)
                v14 = 599;
        }
        a10[v14] = 0;
        if (v26)
        {
            char v22 = 0;
            auto v23 = 0;
            if (v14)
            {
                do
                {
                    if (a10[v23] == 126)
                        ++v22;
                    ++v23;
                }
                while (v23 < v14);
            }
            if (v22 & 1)
                a10[v14 - 1] = 126;
        }
    }
    else
    {
        *a10 = 0;
    }

    ++count;

    if (std::wstring_view(a10) == L"~z~What?")
    {
        _asm nop
    }


    auto str = a10;
    if (!isRecording)
    {
        if (str[0] == L'~' && str[1] == L'z' && str[2] == L'~')
        {
            ToggleRecording();
            isRecording = true;
            count = 0;
        }
    }
    else
    {
        if (str[0] == '\0' || wcscmp(str, lastMsg) != 0)
        {
            if (count >= 300) //dirty hack
            {
                ToggleRecording();
                isRecording = false;
            }
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
            if (count < 300)
            {
                Sleep(300 - count);
                ToggleRecording();
                isRecording = false;
            }

            while (!CheckForLockedFiles())
            {
                Sleep(1);
                if ((GetAsyncKeyState(VK_F1) & 0xF000) != 0)
                    break;
            }
            LogRecordingW2();
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
        }
    }
    wcscpy(lastMsg, str);

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

void InitIV()
{
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
    }; injector::MakeInline<Hack3>(injector::aslr_ptr(0x89E46C), injector::aslr_ptr(0x89E46C + 6));

    injector::MakeCALL(injector::aslr_ptr(0x8AD8DA), sub_8AA3F0, true);
}