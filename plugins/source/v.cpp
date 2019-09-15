#include <windows.h>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <chrono>

#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\utility.hpp"

#include <Trampoline.h>

TrampolineMgr trampolines;

auto start = std::chrono::high_resolution_clock::now();
auto finish = std::chrono::high_resolution_clock::now();

auto gvm = injector::address_manager::singleton();
std::wstring DefaultPathW;
std::vector<DWORD> Keys;
std::ofstream logfileA;
std::wofstream logfileW;
size_t bufSize = 400;

char lastMsg[4000];

bool isRecording = false;
char* m_Message;
uint8_t* bDisplayHud;
uint8_t* bDisplayRadar;
uint8_t* bCutscene;

#include <tlhelp32.h>
#include <RestartManager.h>
#pragma comment(lib ,"Rstrtmgr.lib")

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

DWORD GetRegistryData(std::wstring& str, HKEY key, std::wstring_view subKey, std::wstring_view valueName)
{
    HKEY    hOpenedKey;
    DWORD   dwType;
    DWORD   nMaxLength = MAX_PATH;
    str.resize(nMaxLength);

    if (ERROR_SUCCESS == RegOpenKeyEx(key, subKey.data(), 0, KEY_READ, &hOpenedKey))
    {
        auto rc = RegQueryValueEx(hOpenedKey, valueName.data(), 0, &dwType, (LPBYTE)str.data(), &nMaxLength);
        if (rc != ERROR_SUCCESS)
            return (DWORD)-1;

        RegCloseKey(hOpenedKey);
        str.resize((nMaxLength / sizeof(wchar_t)) - 1);
        return nMaxLength;
    }
    else
    {
        return (DWORD)-1;
    }
}

void GetNVidiaSettings()
{
    GetRegistryData(DefaultPathW, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", L"DefaultPathW");
    DefaultPathW += L"\\Grand Theft Auto V";

    std::wstring temp;
    GetRegistryData(temp, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", L"ManualHKeyCount");
    DWORD ManualHKeyCount = *(DWORD*)temp.data();
    for (size_t i = 0; i < ManualHKeyCount; i++)
    {
        std::wstring k = L"ManualHKey" + std::to_wstring(i);
        GetRegistryData(temp, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", k.data());
        DWORD ManualHKey = *(DWORD*)temp.data();
        Keys.push_back(ManualHKey);
    }
}

std::filesystem::path GetLatestFileName()
{
    std::filesystem::directory_entry latest;
    std::filesystem::file_time_type latest_tm;
    for (auto& p : std::filesystem::directory_iterator(DefaultPathW))
    {
        if (p.path().extension() == ".mp4")
        {
            auto timestamp = std::filesystem::last_write_time(p);
            if (timestamp > latest_tm)
            {
                latest = p;
                latest_tm = timestamp;
            }
        }
    }
    return latest.path().filename();
}

void LogRecordingA()
{
    if (!std::string_view(lastMsg).empty())
    {
        logfileA << GetLatestFileName() << " // " << lastMsg << std::endl;
    }
}

void ToggleRecording()
{
    keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

    for each (auto var in Keys)
    {
        keybd_event(var, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    }

    for each (auto var in Keys)
    {
        keybd_event(var, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
}

injector::hook_back<char*(__fastcall*)(char*, void*, int64_t)> hbsub_140153448;
char* __fastcall sub_140153448(char* a1, void* a2, int64_t a3)
{
    auto r = hbsub_140153448.fun(a1, a2, a3);

    auto str = m_Message;
    if (!isRecording)
    {
        if (str[0] != '\0' && *bCutscene)
        {
            if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~' && str[3] != '\0'))
            {
                ToggleRecording();
                isRecording = true;
                start = std::chrono::high_resolution_clock::now();
            }
        }
    }
    else
    {
        if (str[0] == '\0' || strcmp(str, lastMsg) != 0)
        {
            {
                finish = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = finish - start;

                if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 1.0)
                {
                    ToggleRecording();
                    isRecording = false;
                }
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
                if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() < 1.0)
                {
                    Sleep(1000 - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                    ToggleRecording();
                    isRecording = false;
                }
                Sleep(100);
                while (!CheckForLockedFiles())
                {
                    Sleep(1);
                    if ((GetAsyncKeyState(VK_F1) & 0xF000) != 0)
                        break;
                }
                LogRecordingA();
                DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);

                if (str[0] != '\0' /*&& *bCutscene*/)
                {
                    if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~' && str[3] != '\0'))
                    {
                        ToggleRecording();
                        isRecording = true;
                        start = std::chrono::high_resolution_clock::now();
                    }
                }
            }
        }
    }

    strcpy(lastMsg, str);

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

    return r;
}

injector::hook_back<int64_t(__fastcall*)()> hbsub_140175EE8;
int64_t __fastcall sub_140175EE8()
{
    auto r = hbsub_140175EE8.fun();

    if (isRecording)
    {

        {
            finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 1.0)
            {
                ToggleRecording();
                isRecording = false;
            }
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() < 1.0)
            {
                Sleep(1000 - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                ToggleRecording();
                isRecording = false;
            }
            Sleep(500);
            while (!CheckForLockedFiles())
            {
                Sleep(1);
                if ((GetAsyncKeyState(VK_F1) & 0xF000) != 0)
                    break;
            }
            LogRecordingA();
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
        }

        lastMsg[0] = 0;
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

    return r;
}

void InitV()
{
    auto aslr64 = [](uint64_t p) -> void*
    {
        static uintptr_t module = (uintptr_t)GetModuleHandle(NULL);
        return (void*)((p - 0x140000000) + module);
    };

    m_Message = (char*)aslr64(0x1424699F0);
    bDisplayHud = (uint8_t*)aslr64(0x140000000 + 0x1F36BC4);
    bDisplayRadar = (uint8_t*)aslr64(0x140000000 + 0x1F36BC0);
    //bCutscene = (uint8_t*)aslr64(0x140000000 + 0x1B60288);
    bCutscene = (uint8_t*)aslr64(0x140000000 + 0x23CC4FA);
    //GTA5.exe+1DAEA21
    //GTA5.exe+1DAEA22
    //GTA5.exe+1F42E4F
    //GTA5.exe+23CC4FA

    Trampoline& trampoline = trampolines.MakeTrampoline(GetModuleHandle(NULL));

    hbsub_140153448.fun = injector::MakeCALL(aslr64(0x140CF9051), trampoline.Jump(sub_140153448)).get();
    hbsub_140175EE8.fun = injector::MakeCALL(aslr64(0x140CFA0D1), trampoline.Jump(sub_140175EE8)).get();
}

void Init()
{
    GetNVidiaSettings();
    logfileA.open(DefaultPathW + L"\\log.txt", std::ios_base::app);

    InitV();
}

template<class T>
T GetModulePath(HMODULE hModule)
{
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;
    T ret;
    auto bufferSize = INITIAL_BUFFER_SIZE;
    for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
    {
        ret.resize(bufferSize);
        size_t charsReturned = 0;
        if constexpr (std::is_same_v<T, std::string>)
            charsReturned = GetModuleFileNameA(hModule, &ret[0], bufferSize);
        else
            charsReturned = GetModuleFileNameW(hModule, &ret[0], bufferSize);
        if (charsReturned < ret.length())
        {
            ret.resize(charsReturned);
            return ret;
        }
        else
        {
            bufferSize *= 2;
        }
    }
    return T();
}

template<class T>
T GetThisModulePath()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetResolutionsList, &hm);
    T r = GetModulePath<T>(hm);
    if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

template<class T>
T GetThisModuleName()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetResolutionsList, &hm);
    const T moduleFileName = GetModulePath<T>(hm);
    if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

template<class T>
T GetExeModulePath()
{
    T r = GetModulePath<T>(NULL);
    if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

template<class T>
T GetExeModuleName()
{
    const T moduleFileName = GetModulePath<T>(NULL);
    if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        Init();
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        InitializeASI();
    }
    return TRUE;
}