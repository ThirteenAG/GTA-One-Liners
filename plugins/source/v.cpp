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

#include "Hooking.Patterns.h"

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
    GetRegistryData(temp, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", L"DVRHKeyCount");
    DWORD ManualHKeyCount = *(DWORD*)temp.data();
    for (size_t i = 0; i < ManualHKeyCount; i++)
    {
        std::wstring k = L"DVRHKey" + std::to_wstring(i);
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

void LogRecordingA(std::string s)
{
    logfileA << /*GetLatestFileName() <<*/ s << " // " << lastMsg << std::endl;
}

void SaveRecording()
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

    keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

injector::hook_back<char*(__fastcall*)(char*, void*, int64_t)> hbsub_140153448;
char* __fastcall sub_140153448(char* a1, void* a2, int64_t a3)
{
    auto r = hbsub_140153448.fun(a1, a2, a3);

    auto str = m_Message;
    if (!isRecording)
    {
        if (str[0] != '\0')
        {
            if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~' && str[3] != '\0'))
            {
                isRecording = true;
                start = std::chrono::high_resolution_clock::now();
            }
        }
    }
    else
    {
        if (str[0] == '\0' || strcmp(str, lastMsg) != 0)
        {
            finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;
            std::string out(std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));

            isRecording = false;

            SaveRecording();
            LogRecordingA(out);


            if (str[0] != '\0')
            {
                if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~' && str[3] != '\0'))
                {
                    isRecording = true;
                    start = std::chrono::high_resolution_clock::now();
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
        finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::string out(std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));

        isRecording = false;

        SaveRecording();
        LogRecordingA(out);
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
    //auto aslr64 = [](uint64_t p) -> void*
    //{
    //    static uintptr_t module = (uintptr_t)GetModuleHandle(NULL);
    //    return (void*)((p - 0x140000000) + module);
    //};
    //
    //m_Message = (char*)aslr64(0x1424699F0);
    //bDisplayHud = (uint8_t*)aslr64(0x140000000 + 0x1F36BC4);
    //bDisplayRadar = (uint8_t*)aslr64(0x140000000 + 0x1F36BC0);
    //
    //Trampoline& trampoline = trampolines.MakeTrampoline(GetModuleHandle(NULL));
    //
    //hbsub_140153448.fun = injector::MakeCALL(aslr64(0x140CF9051), trampoline.Jump(sub_140153448)).get();
    //hbsub_140175EE8.fun = injector::MakeCALL(aslr64(0x140CFA0D1), trampoline.Jump(sub_140175EE8)).get();

    auto pattern = hook::pattern("80 3D ? ? ? ? ? 74 0C C6 05 ? ? ? ? ? E8 ? ? ? ? 48 83 C4 28 C3");
    m_Message = (char*)((uintptr_t)pattern.get_first(0) + *pattern.get_first<uint32_t>(2) + 7);

    pattern = hook::pattern("8B 05 ? ? ? ? 3B C6 0F 45 C6 89 05 ? ? ? ? 8B 05 ? ? ? ? 3B C6 0F 45 C6 89 05 ? ? ? ? 8B 05 ? ? ? ? 3B C6 0F 45 C6 89 05 ? ? ? ? 8B 05 ? ? ? ? 3B C6 0F 45 C6 89 05 ? ? ? ? E8 ? ? ? ? 44 8D 4E 06 84 C0");
    bDisplayHud = (uint8_t*)((uintptr_t)pattern.get_first(0) + *pattern.get_first<uint32_t>(2) + 6);

    pattern = hook::pattern("83 25 ? ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8B CB E8 ? ? ? ? E8 ? ? ? ? 48 83 3D ? ? ? ? ? 74 10 48 83 25 ? ? ? ? ? 48 83 25 ? ? ? ? ? 48 8D 0D ? ? ? ? C6 05 ? ? ? ? ? 48 83 C4 20 5B E9");
    bDisplayRadar = (uint8_t*)((uintptr_t)pattern.get_first(0) + *pattern.get_first<uint32_t>(2) + 7);

    Trampoline& trampoline = trampolines.MakeTrampoline(GetModuleHandle(NULL));

    pattern = hook::pattern("41 B8 ? ? ? ? E8 ? ? ? ? 48 8B CB E8 ? ? ? ? 48 83 C4 20 5B C3 ");
    hbsub_140153448.fun = injector::MakeCALL(pattern.get_first(6), trampoline.Jump(sub_140153448)).get(); //sub_140153448 = sub_10053B0 in max payne 3

    pattern = hook::pattern("C6 05 ? ? ? ? ? E8 ? ? ? ? 4C 8D 9C 24 ? ? ? ? 49 8B 5B 10 49 8B 73 18 49 8B 7B 20 49 8B E3 41 5E C3");
    hbsub_140175EE8.fun = injector::MakeCALL(pattern.get_first(7), trampoline.Jump(sub_140175EE8)).get();
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