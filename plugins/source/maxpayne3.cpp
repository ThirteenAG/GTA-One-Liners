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

extern void SaveRecording2();
extern wchar_t lastMsg[4000];

auto _start = std::chrono::high_resolution_clock::now();
auto _finish = std::chrono::high_resolution_clock::now();

void SaveRecording()
{
    //keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    //keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

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

void LogRecordingW3()
{
    if (!std::wstring_view(lastMsg).empty())
    {
        logfileW << GetLatestFileName() << L" // " << lastMsg << std::endl;
    }
}

void LogRecordingW4(std::wstring s)
{
    logfileW << /*GetLatestFileName() <<*/ s << " // " << lastMsg << std::endl;
}

extern void DoSuspendThread(DWORD targetProcessId, DWORD targetThreadId, bool action);

extern BOOL CheckForFileLock(LPCWSTR pFilePath, bool bReleaseLock = false);


extern bool CheckForLockedFiles();

int __cdecl sub_19EAF50(char* a1, wchar_t* a2)
{
    char v2; // al
    __int16 v4; // ax
    __int16 v5; // cx
    __int16 v6; // ax

    v2 = *a1;
    if (*a1 >= 0)
    {
        *a2 = v2;
        return 1;
    }
    if ((v2 & 0xE0) == 0xC0)
    {
        if (a1[1])
        {
            v4 = (v2 & 0x1F) << 6;
            *a2 = v4;
            *a2 = v4 | a1[1] & 0x3F;
            return 2;
        }
    }
    else if ((v2 & 0xF0) == 0xE0 && a1[1] && a1[2])
    {
        v5 = v2 << 12;
        *a2 = v5;
        v6 = v5 | ((a1[1] & 0x3F) << 6);
        *a2 = v6;
        *a2 = v6 | a1[2] & 0x3F;
        return 3;
    }
    return 0;
}

void __cdecl sub_12D07F0(char* a1, wchar_t* a2)
{
    int v3; // edi

    if (a1)
    {
        v3 = 0;
        if (*a1)
        {
            do
            {
                v3 += sub_19EAF50(a1, a2);
                a1 += v3;
                ++a2;
            }
            while (a1[v3]);
        }
        *a2 = 0;
    }
}

uint8_t* reg_esi;
void DisplayHud(bool bShow)
{
    if (reg_esi)
    {
        if (bShow)
        {
            *(reg_esi + 0x147) = 0;
        }
        else
        {
            *(reg_esi + 0x147) = 1;
        }
    }
}

extern size_t count;
int __cdecl sub_123C600(wchar_t* a1, int a2, int a3, int a4, int a5, int a6, int a7, float a8, int a9, wchar_t* a10)
{
    wchar_t* v10; // edi
    int result; // eax
    unsigned int v12; // ebx
    unsigned int v13; // esi
    wchar_t* v14; // eax
    int v15; // edx
    unsigned int v16; // ebp
    wchar_t* v17; // ecx
    __int16 v18; // ax
    char* v19; // edi
    char* v20; // edi
    wchar_t* v21; // esi
    int v22; // ecx
    int v23; // edx
    char v24; // [esp+Fh] [ebp-2Dh]
    int v25; // [esp+10h] [ebp-2Ch]
    unsigned int v26; // [esp+14h] [ebp-28h]
    wchar_t* v27; // [esp+18h] [ebp-24h]
    char src[12]; // [esp+1Ch] [ebp-20h] BYREF
    wchar_t dst[5]; // [esp+28h] [ebp-14h] BYREF

    v10 = a1;
    if (a1)
    {
        v24 = 0;
        if (a9 < 0)
            sprintf(src, "%d", a2);
        else
            sprintf(src, "%.*f", a9, a8);
        v12 = strlen(src);
        sub_12D07F0(src, dst);
        v13 = 0;
        v26 = 0;
        v14 = a1;
        if (*a1)
        {
            do
            {
                ++v14;
                ++v13;
            }
            while (*v14);
            v26 = v13;
        }
        v15 = 0;
        v16 = 0;
        v25 = 0;
        if (v13)
        {
            v17 = a1 + 2;
            do
            {
                v18 = v10[v16];
                if (v18 == 126 && v10[v16 + 1] == 49 && *v17 == 126 || v18 == 124 && v10[v16 + 1] == 48 && *v17 == 92)
                {
                    v16 += 3;
                    v27 = v17 + 3;
                    if (v12)
                    {
                        v19 = (char*)&a10[v15];
                        memcpy(v19, dst, 4 * (v12 >> 1));
                        v21 = &dst[v12 >> 1];
                        v20 = &v19[4 * (v12 >> 1)];
                        v22 = v12 & 1;
                        v23 = v12 + v15;
                        while (v22)
                        {
                            *(WORD*)v20 = *v21++;
                            v20 += 2;
                            --v22;
                        }
                        v10 = a1;
                        v25 = v23;
                    }
                    switch (++v24)
                    {
                    case 1:
                        sprintf(src, "%d", a3);
                        break;
                    case 2:
                        sprintf(src, "%d", a4);
                        break;
                    case 3:
                        sprintf(src, "%d", a5);
                        break;
                    case 4:
                        sprintf(src, "%d", a6);
                        break;
                    case 5:
                        sprintf(src, "%d", a7);
                        break;
                    default:
                        break;
                    }
                    v12 = strlen(src);
                    sub_12D07F0(src, dst);
                    v17 = v27;
                    v15 = v25;
                    v13 = v26;
                }
                else
                {
                    a10[v15] = v18;
                    v10 = a1;
                    ++v15;
                    ++v16;
                    v25 = v15;
                    ++v17;
                }
            }
            while (v16 < v13);
        }
        result = 0;
        a10[v15] = 0;
    }
    else
    {
        result = 0;
        *a10 = 0;
    }

    ++count;


    auto strip = [](std::wstring& s)
    {
        int j = 0;
        for (int i = 0; i < s.size(); i++)
        {
            if ((s[i] >= L'A' && s[i] <= L'Z') ||
                    (s[i] >= L'a' && s[i] <= L'z'))
            {
                s[j] = s[i];
                j++;
            }
        }
        s = s.substr(0, j);
    };

    auto s = std::wstring(a10);
    if (s.length() > 3 && s[0] == L'~' && s[1] == L'z' && s[2] == L'~')
    {
        if (s.starts_with(L"~z~Chapter"))
            return 0;
        if (s.starts_with(L"~z~Golgotha Cemetary"))
            return 0;
        if (s.starts_with(L"~z~4:00 PM"))
            return 0;

        s = s.substr(3);
        strip(s);
        if (std::all_of(s.begin(), s.end(), [](unsigned short c) { return std::isupper(c); }))
        {
            return 0;
        }
    }


    auto str = a10;
    if (!isRecording)
    {
        if (str[0] != '\0')
        {
            if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~' && str[3] != '\0'))
            {
                isRecording = true;
                _start = std::chrono::high_resolution_clock::now();
            }
        }
    }
    else
    {
        if (str[0] == '\0' || wcscmp(str, lastMsg) != 0)
        {
            _finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = _finish - _start;
            std::wstring out(std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));

            isRecording = false;

            SaveRecording();
            LogRecordingW4(out);


            if (str[0] != '\0')
            {
                if ((str[0] == L'~' && str[1] == L'z' && str[2] == L'~' && str[3] != '\0'))
                {
                    isRecording = true;
                    _start = std::chrono::high_resolution_clock::now();
                }
            }
        }
    }

    wcscpy(lastMsg, str);

    if (isRecording)
    {
        DisplayHud(false);
    }
    else
    {
        DisplayHud(true);
    }


//if (std::wstring_view(a10) == L"~z~What?")
//{
//    _asm nop
//}

    /*
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
            LogRecordingW3();
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
        }
    }
    wcscpy(lastMsg, str);

    if (isRecording)
    {
        DisplayHud(false);
    }
    else
    {
        DisplayHud(true);
    }
    */

    return result;
}

void __fastcall outline_size(float* _this, void* edx, float a2)
{
    _this[8] = 1.5f / 512.0f;
    _this[13] = 1.5f / 512.0f;
}

void InitMaxPayne3()
{
    auto pattern = hook::pattern("C6 44 24 ? ? F3 0F 11 44 24 ? F3 0F 11 44 24 ? 89 4C 24 54");
    struct Hack4
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.esp + 0x7A) = 1;
            *(float*)(regs.esp + 0x24) *= 2.0f;
            *(float*)(regs.esp + 0x28) *= 2.0f;
        }
    }; injector::MakeInline<Hack4>(pattern.get_first(5));

    pattern = hook::pattern("F3 0F 11 44 24 ? 39 58 30 75 08 F3 0F 10 44 24 ? EB 03 0F 28 C1 8B 40 08");
    struct Hack5
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x30) = *(float*)(regs.eax + 0x20) * 2.0f;
            *(float*)(regs.esp + 0x34) = *(float*)(regs.eax + 0x24) * 2.0f;
        }
    }; injector::MakeInline<Hack5>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("E8 ? ? ? ? 8B 4E 08 6A FF");
    injector::MakeCALL(pattern.get_first(0), sub_123C600, true); //0x0123DDA4

    pattern = hook::pattern("0F B6 96 ? ? ? ? 8B 01 8B 80 ? ? ? ? 52 FF D0 8B 8E ? ? ? ? 0F B6 86");
    struct HackHud
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)&regs.edx = *(uint8_t*)(regs.esi + 0x14F);
            reg_esi = (uint8_t*)(regs.esi);
        }
    }; injector::MakeInline<HackHud>(pattern.get_first(0), pattern.get_first(7));

    //remove LOADING... and skip button
    pattern = hook::pattern("83 EC 44 56 57 8B F1 8B 0D ? ? ? ? 6A 1F");
    injector::MakeRET(pattern.get_first(0));
    pattern = hook::pattern("E8 ? ? ? ? D9 EE 8B 86 ? ? ? ? 8B 38 83 EC 08 D9 54 24 04 8D 4C 24 20 D9 1C 24 E8 ? ? ? ? F3 0F 7E 00 8B 97 ? ? ? ? 83 EC 18 8B CC 66 0F D6 01 F3 0F 7E 40 ? 66 0F D6 41 ? F3 0F 7E 40 ? 6A 18");
    injector::MakeNOP(pattern.get_first(0), 5, true); //proper way prolly

    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 8D 94 24 ? ? ? ? 6A 02");
    injector::MakeCALL(pattern.get_first(0), outline_size, true);

    pattern = hook::pattern("D9 1C 24 E8 ? ? ? ? 68 ? ? ? ? 68");
    injector::MakeCALL(pattern.get_first(3), outline_size, true);

    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 8D 44 24 20 6A 03 50 88 1D ? ? ? ? C6 05 ? ? ? ? ? 88 1D ? ? ? ? E8 ? ? ? ? 8B 08");
    injector::MakeCALL(pattern.get_first(0), outline_size, true);

    pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 44 24 ? 8D 4C 24 30 F3 0F 11 05 ? ? ? ? F3 0F 10 44 24 ? 51 B9 ? ? ? ? F3 0F 11 05 ? ? ? ? E8 ? ? ? ? 8A 5C 24 0F 80 FB FF 75 0C");
    injector::MakeCALL(pattern.get_first(0), outline_size, true);
}