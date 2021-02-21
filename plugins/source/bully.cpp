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
#include "injector\assembly.hpp"
#include "injector\utility.hpp"

#include "Hooking.Patterns.h"

auto start = std::chrono::high_resolution_clock::now();
auto finish = std::chrono::high_resolution_clock::now();

extern std::wstring DefaultPathW;
extern std::vector<DWORD> Keys;
extern std::ofstream logfileA;
extern std::wofstream logfileW;

extern wchar_t lastMsg[4000];

extern bool isRecording;
wchar_t* m_Message;
extern uint8_t* bDisplayHud;
extern uint8_t* bDisplayRadar;
uint8_t* bCutscene;

#include <tlhelp32.h>
#include <RestartManager.h>
#pragma comment(lib ,"Rstrtmgr.lib")

DWORD GetRegistryData2(std::wstring& str, HKEY key, std::wstring_view subKey, std::wstring_view valueName)
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

void GetNVidiaSettings2()
{
    Keys.clear();
    GetRegistryData2(DefaultPathW, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", L"DefaultPathW");
    DefaultPathW += L"\\Grand Theft Auto  San Andreas";

    std::wstring temp;
    GetRegistryData2(temp, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", L"DVRHKeyCount");
    DWORD ManualHKeyCount = *(DWORD*)temp.data();
    for (size_t i = 0; i < ManualHKeyCount; i++)
    {
        std::wstring k = L"DVRHKey" + std::to_wstring(i);
        GetRegistryData2(temp, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", k.data());
        DWORD ManualHKey = *(DWORD*)temp.data();
        Keys.push_back(ManualHKey);
    }
}

void LogRecordingW(std::wstring s)
{
    logfileW << /*GetLatestFileName() <<*/ s << L" // " << lastMsg << std::endl;
}

void SaveRecording2()
{
    keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

    for (auto var : Keys)
    {
        keybd_event(var, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    }

    for (auto var : Keys)
    {
        keybd_event(var, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }

    //keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    //keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

bool isSubtitle = false;
injector::hook_back<void(__cdecl*)(char*, void*)> hbsub_6900C0;
void __cdecl sub_6900C0(char* a1, void* a2)
{
    hbsub_6900C0.fun(a1, a2);

    auto str = m_Message;
    if (isSubtitle)
    {
        if (!isRecording)
        {
            if (str[0] != L'\0')
            {
                if (wcschr(str, L'~') == NULL && *bCutscene)
                {
                    isRecording = true;
                    start = std::chrono::high_resolution_clock::now();
                }
            }
        }
        else
        {
            if (str[0] == L'\0' || wcscmp(str, lastMsg) != 0)
            {
                finish = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = finish - start;
                std::wstring out(std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));

                isRecording = false;

                SaveRecording2();
                LogRecordingW(out);

                if (str[0] != '\0' && wcscmp(str, lastMsg) != 0)
                {
                    if (wcschr(str, L'~') == NULL && *bCutscene)
                    {
                        isRecording = true;
                        start = std::chrono::high_resolution_clock::now();
                    }
                }
            }
        }

        wcscpy(lastMsg, str);
    }
}

injector::hook_back<void(__cdecl*)(void*, void*)> hbsub_688ED0;
void __cdecl sub_688ED0(void* a1, void* a2)
{
    isSubtitle = true;
    hbsub_688ED0.fun(a1, a2);
    isSubtitle = false;
}

void InitBully2()
{
    m_Message = (wchar_t*)0x01B89980;

    hbsub_6900C0.fun = injector::MakeCALL(0x688EF3, sub_6900C0).get();
    hbsub_6900C0.fun = injector::MakeCALL(0x68911D, sub_6900C0).get();
    hbsub_6900C0.fun = injector::MakeCALL(0x68AFF8, sub_6900C0).get();
    hbsub_6900C0.fun = injector::MakeCALL(0x68B048, sub_6900C0).get();
    hbsub_6900C0.fun = injector::MakeCALL(0x68B0A8, sub_6900C0).get();
    hbsub_6900C0.fun = injector::MakeCALL(0x68BAFF, sub_6900C0).get();
    hbsub_6900C0.fun = injector::MakeCALL(0x68D4D3, sub_6900C0).get();

    hbsub_688ED0.fun = injector::MakeCALL(0x55B32C, sub_688ED0).get();

    bCutscene = (uint8_t*)0x20C5BE4;

    static std::wstring prevMessage;
    struct Hack0
    {
        void operator()(injector::reg_pack& regs)
        {
            auto str = m_Message;
            if (isRecording)
            {
                if (str[0] == L'\0')
                {
                    finish = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed = finish - start;
                    std::wstring out(std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));

                    isRecording = false;

                    SaveRecording2();
                    LogRecordingW(out);
                }
            }

            if (regs.edi == 0x000000c8)
            {
                str[0] = 0;
            }
        }
    }; injector::MakeInline<Hack0>(0x53D003);
    injector::MakeRET(0x53D003+5);


    //struct Hack1
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        *(uint8_t*)(regs.ebx + 0xA1) = 0;
    //
    //        if (*bCutscene)
    //            return;
    //
    //        if (isRecording)
    //        {
    //            finish = std::chrono::high_resolution_clock::now();
    //            std::chrono::duration<double> elapsed = finish - start;
    //            std::wstring out(std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));
    //
    //            isRecording = false;
    //
    //            SaveRecording2();
    //            LogRecordingW(out);
    //        }
    //    }
    //}; injector::MakeInline<Hack1>(0x68F32F, 0x68F32F + 7);
    //
    //struct Hack2
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        *(uint8_t*)(regs.edi + 0xA1) = 1;
    //
    //        if (*bCutscene)
    //            return;
    //
    //        auto str = m_Message;
    //        if (!isRecording)
    //        {
    //            if (str[0] != L'\0')
    //            {
    //                //if (wcschr(str, L'~') == NULL)
    //                {
    //                    isRecording = true;
    //                    start = std::chrono::high_resolution_clock::now();
    //                }
    //            }
    //        }
    //    }
    //}; injector::MakeInline<Hack2>(0x68F1C3, 0x68F1C3 + 7);

}

void InitBully()
{
    GetNVidiaSettings2();

    InitBully2();
}
