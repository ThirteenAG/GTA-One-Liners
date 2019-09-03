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

static constexpr uintptr_t EErec_start = 0x30000000;
static constexpr uintptr_t EErec_end = 0x34000000;

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

wchar_t lastMsg[4000];

void LogRecordingW2()
{
    if (!std::wstring_view(lastMsg).empty())
    {
        logfileW << GetLatestFileName() << L" // " << lastMsg << std::endl;
    }
}

void __cdecl SetMessageW2(wchar_t *str)
{
    if (str[0] == 0 || (wcscmp(str, lastMsg) != 0))
    {
        if (isRecording)
        {
            //ToggleRecording();
            //Sleep(500);
            //isRecording = false;
            //LogRecordingW2();

            ToggleRecording();
            isRecording = false;
            void DoSuspendThread(DWORD targetProcessId, DWORD targetThreadId, bool action);
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), true);
            Sleep(500);
            LogRecordingW2();
            DoSuspendThread(GetCurrentProcessId(), GetCurrentThreadId(), false);
        }
    }
    else if (wcschr(str + 7, L'~') == nullptr && ((str[0] == L'~' && str[1] == L'w' && str[2] == L'~') || (str[0] != L'~' && str[2] != L'~')))
    {
        if (wcschr(str + 7, L'$') == nullptr && wcsstr(str, L"STUNT") == nullptr && wcsstr(str, L"Distance:") == nullptr && wcsstr(str, L"Cost:") == nullptr)
        {
            if (!isRecording)
            {
                ToggleRecording();
                isRecording = true;
            }
        }
    }

    wcscpy((wchar_t*)lastMsg, (wchar_t*)str);

    if (bDisplayHud && bDisplayRadar)
    {
        if (isRecording)
        {
            *bDisplayHud = 0;
            *bDisplayRadar = 2;
        }
        else
        {
            *bDisplayHud = 1;
            *bDisplayRadar = 0;
        }
    }
}

bool __stdcall memory_readable(void *ptr, size_t byteCount)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
        return false;

    size_t blockOffset = (size_t)((char *)ptr - (char *)mbi.AllocationBase);
    size_t blockBytesPostPtr = mbi.RegionSize - blockOffset;

    if (blockBytesPostPtr < byteCount)
        return memory_readable((char *)ptr + blockBytesPostPtr,
            byteCount - blockBytesPostPtr);

    return true;
}

void VCS()
{
    while (!memory_readable((void*)EErec_start, 4))
        std::this_thread::yield();

    auto pattern = hook::pattern(EErec_start, EErec_end, "C7 05 ? ? ? ? ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 \
                                                          ? ? ? ? A1 ? ? ? ? D1 E0 99 A3 ? ? ? ? 89 15 ? ? ? ? A1 ? ? ? ? 03 05 ? ? ? ? 99 A3 \
                                                          ? ? ? ? 89 15 ? ? ? ? 31 D2 8B 0D ? ? ? ? 83 C1"); //0x30234072

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr1 = *pattern.get_first<uint32_t*>(2);
    static auto val1 = *pattern.get_first<uint32_t>(6);
    struct SetMessageHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *ptr1 = val1;

            static char* m_Message2 = nullptr;

            if (m_Message == nullptr)
                m_Message = (char*)regs.ecx;

            if (m_Message2 == nullptr && m_Message != (char*)regs.ecx)
                m_Message2 = (char*)regs.ecx;

            if (regs.ecx == (uint32_t)m_Message || regs.ecx == (uint32_t)m_Message2)
                SetMessageW2((wchar_t*)m_Message);

        }
    }; injector::MakeInline<SetMessageHook1>(pattern.get_first(0), pattern.get_first(10));

    pattern = hook::pattern(EErec_start, EErec_end, "8B 35 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 81 C1 ? ? ? ? 89 C8 C1 E8 ? 8B 04 85 \
                                                     ? ? ? ? BB ? ? ? ? 01 C1 0F 88 ? ? ? ? 88 11 89 35 ? ? ? ? A1 ? ? ? ? 83 C0 ?  \
                                                     A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? BA"); //0x302340EB

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr2 = *pattern.get_first<uint32_t*>(2);
    struct SetMessageHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *ptr2;

            auto str = (wchar_t*)regs.ecx;

            if (str[-1] == 0)
                m_Message = (char*)regs.ecx;

            static char* m_Message2 = nullptr;
            if (m_Message2 == nullptr && m_Message != (char*)regs.ecx)
                m_Message2 = (char*)regs.ecx;

            if (regs.ecx == (uint32_t)m_Message || regs.ecx == (uint32_t)m_Message2)
                SetMessageW2((wchar_t*)m_Message);

        }
    }; injector::MakeInline<SetMessageHook2>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern(EErec_start, EErec_end, "8B 35 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 81 C1 ? ? ? ? 89 C8 C1 E8 ? 8B 04 85 \
                                                     ? ? ? ? BB ? ? ? ? 01 C1 0F 88 ? ? ? ? 88 11 89 35 ? ? ? ? A1 ? ? ? ? 83 C0 ? \
                                                     A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? BA ? ? ? ? 8B 0D ? ? ? ? 81 C1"); //0x307E342A

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr22 = *pattern.get_first<uint32_t*>(2);
    struct SetMessageHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *ptr22;

            auto str = (wchar_t*)regs.ecx;

            if (str[-1] == 0)
                m_Message = (char*)regs.ecx;

            static char* m_Message2 = nullptr;
            if (m_Message2 == nullptr && m_Message != (char*)regs.ecx)
                m_Message2 = (char*)regs.ecx;

            if (regs.ecx == (uint32_t)m_Message || regs.ecx == (uint32_t)m_Message2)
                SetMessageW2((wchar_t*)m_Message);

        }
    }; injector::MakeInline<SetMessageHook3>(pattern.get_first(0), pattern.get_first(6));



    pattern = hook::pattern(EErec_start, EErec_end, "89 15 ? ? ? ? A1 ? ? ? ? 99 A3 ? ? ? ? 89 15 ? ? ? ? 31 C0 A3 ? ? ? ? A3 \
                                                     ? ? ? ? A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? \
                                                     ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? \
                                                     ? ? 0F 88 ? ? ? ? E9 ? ? ? ? 8B 35"); //0x30217F5C

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto sub_scale = *pattern.get_first<float*>(2);
    struct SubtitleScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *sub_scale = *(float*)&regs.edx;

            if (regs.edx == 0x3f1c28f6) //0.61
                *sub_scale *= 1.8f;
        }
    }; injector::MakeInline<SubtitleScaleHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern(EErec_start, EErec_end, "A3 ? ? ? ? 89 15 ? ? ? ? 89 35 ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? \
                                                     ? 0F 88 ? ? ? ? E9 ? ? ? ? 83 3D ? ? ? ? ? 0F 85 ? ? ? ? 83 3D ? ? ? ? ? 0F 85 ? \
                                                     ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 \
                                                     ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? A1 ? ? ? ? 8B \
                                                     15 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 \
                                                     ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? A3 ? ? ? \
                                                     ? 89 15 ? ? ? ? 31 C0 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 ? \
                                                     ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? 81 05"); //0x3062B475

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr3 = *pattern.get_first<uint32_t*>(1);
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *ptr3 = regs.eax;
            bDisplayHud = (uint8_t*)regs.ecx;
        }
    }; injector::MakeInline<HudHook>(pattern.get_first(0));


    pattern = hook::pattern(EErec_start, EErec_end, "A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 35 ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? \
                                                     0F 88 ? ? ? ? E9 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 83 3D ? ? ? ? ? 0F 85 \
                                                     ? ? ? ? 83 3D ? ? ? ? ? 0F 85 ? ? ? ? BA"); //0x305EEDFD

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr4 = *pattern.get_first<uint32_t*>(1);
    struct RadarHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *ptr3 = regs.eax;
            bDisplayRadar = (uint8_t*)regs.ecx;
        }
    }; injector::MakeInline<RadarHook>(pattern.get_first(0));
}

void LCS()
{
    Sleep(10000);

    while (!memory_readable((void*)EErec_start, 4))
        std::this_thread::yield();

    auto pattern = hook::pattern(EErec_start, EErec_end, "C7 05 ? ? ? ? ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 \
                                                          ? ? ? ? E9 ? ? ? ? A1 ? ? ? ? 05 ? ? ? ? 99 A3 ? ? ? ? 89 15 ? ? ? ? A1 \
                                                          ? ? ? ? D1 E0 99 A3 ? ? ? ? 89 15 ? ? ? ? A1 ? ? ? ? 03 05"); //0x306F3349

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr1 = *pattern.get_first<uint32_t*>(2);
    static auto val1 = *pattern.get_first<uint32_t>(6);
    struct SetMessageHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *ptr1 = val1;

            static char* m_Message2 = nullptr;

            if (m_Message == nullptr)
                m_Message = (char*)regs.ecx;

            if (m_Message2 == nullptr && m_Message != (char*)regs.ecx)
                m_Message2 = (char*)regs.ecx;

            if (regs.ecx == (uint32_t)m_Message || regs.ecx == (uint32_t)m_Message2)
                SetMessageW2((wchar_t*)m_Message);

        }
    }; injector::MakeInline<SetMessageHook1>(pattern.get_first(0), pattern.get_first(10));

    pattern = hook::pattern(EErec_start, EErec_end, "89 35 ? ? ? ? 31 C0 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? BA ? ? ? ? 8B 0D ? ? ? ? 81 C1"); //0x30612BB4

    while (!pattern.clear().count_hint(1).size())
        std::this_thread::yield();

    static auto ptr2 = *pattern.get_first<uint32_t*>(2);
    struct SetMessageHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *ptr2 = regs.esi;

            auto str = (wchar_t*)regs.ecx;

            if (str[-1] == 0)
                m_Message = (char*)regs.ecx;

            static char* m_Message2 = nullptr;
            if (m_Message2 == nullptr && m_Message != (char*)regs.ecx)
                m_Message2 = (char*)regs.ecx;

            if (regs.ecx == (uint32_t)m_Message || regs.ecx == (uint32_t)m_Message2)
                SetMessageW2((wchar_t*)m_Message);
        }
    }; injector::MakeInline<SetMessageHook2>(pattern.get_first(0), pattern.get_first(6));


    //pattern = hook::pattern(EErec_start, EErec_end, "89 15 ? ? ? ? 89 35 ? ? ? ? 31 C0 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 \
    //                                                 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? A1 ? ? ? ? 05"); //0x301FDDF9
    //
    //while (!pattern.clear().count_hint(1).size())
    //    std::this_thread::yield();
    //
    //static auto sub_scaleX = *pattern.get_first<float*>(2);
    //struct SubtitleScaleXHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        *sub_scaleX = *(float*)&regs.edx;
    //
    //        if (regs.edx == 0x3eb0abb4) //
    //            *sub_scaleX *= 1.8f;
    //    }
    //}; injector::MakeInline<SubtitleScaleXHook>(pattern.get_first(0), pattern.get_first(6));
    //
    //pattern = hook::pattern(EErec_start, EErec_end, "89 15 ? ? ? ? 8B 35 ? ? ? ? 8B 15 ? ? ? ? E8"); //0x301FDDE7
    //
    //while (!pattern.clear().count_hint(1).size())
    //    std::this_thread::yield();
    //
    //static auto sub_scaleY = *pattern.get_first<float*>(2);
    //struct SubtitleScaleYHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        *sub_scaleY = *(float*)&regs.edx;
    //
    //        if (regs.edx == 0x3f35c28f) //
    //            *sub_scaleY *= 1.8f;
    //    }
    //}; injector::MakeInline<SubtitleScaleYHook>(pattern.get_first(0), pattern.get_first(6));

    static bool bOnce = false;
    if (!bOnce)
    {
        pattern = hook::pattern(EErec_start, EErec_end, "A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 8B 35 ? ? ? ? B8 ? ? ? ? 83 3D ? ? ? ? ? 77 ? 72 ? 83 3D \
                                                     ? ? ? ? ? 77 ? 31 C0 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 35 ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 \
                                                     ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? 83 3D ? ? ? ? ? 0F 85 ? ? ? ? 83 3D ? ? ? ? \
                                                     ? 0F 85 ? ? ? ? 8B 0D ? ? ? ? 81 C1 ? ? ? ? 89 C8 C1 E8 ? 8B 04 85 ? ? ? ? BB ? ? ? ? 01 C1 \
                                                     0F 88 ? ? ? ? 0F B6"); //0x30612AC4

        while (!pattern.clear().count_hint(1).size())
            std::this_thread::yield();

        static auto ptr3 = *pattern.get_first<uint32_t*>(1);
        struct HudHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *ptr3 = regs.eax;
                bDisplayHud = (uint8_t*)regs.ecx;
            }
        }; injector::MakeInline<HudHook>(pattern.get_first(0));


        pattern = hook::pattern(EErec_start, EErec_end, "A3 ? ? ? ? 89 15 ? ? ? ? 89 35 ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 \
                                                     ? ? ? ? E9 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 83 3D ? ? ? ? ? 0F 85 ? ? \
                                                     ? ? 83 3D ? ? ? ? ? 0F 85 ? ? ? ? 8B 0D"); //0x3061EF6D

        while (!pattern.clear().count_hint(1).size())
            std::this_thread::yield();

        static auto ptr4 = *pattern.get_first<uint32_t*>(1);
        struct RadarHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *ptr3 = regs.eax;
                bDisplayRadar = (uint8_t*)regs.ecx;
            }
        }; injector::MakeInline<RadarHook>(pattern.get_first(0));

        bOnce = true;
    }

    pattern = hook::pattern(EErec_start, EErec_end, "89 35 ? ? ? ? 31 C0 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? A1 ? ? ? ? 83 C0 ? A3 ? ? ? ? 2B 05 ? ? ? ? 0F 88 ? ? ? ? E9 ? ? ? ? BA ? ? ? ? 8B 0D ? ? ? ? 81 C1"); //0x30612BB4

    while (!pattern.clear().count_hint(4).size())
        std::this_thread::yield();

    static auto ptr22 = *pattern.get_first<uint32_t*>(2);
    struct SetMessageHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *ptr22 = regs.esi;

            auto str = (wchar_t*)regs.ecx;

            if (str[-1] == 0)
                m_Message = (char*)regs.ecx;

            static char* m_Message2 = nullptr;
            if (m_Message2 == nullptr && m_Message != (char*)regs.ecx)
                m_Message2 = (char*)regs.ecx;

            if (regs.ecx == (uint32_t)m_Message || regs.ecx == (uint32_t)m_Message2)
                SetMessageW2((wchar_t*)m_Message);
        }
    }; injector::MakeInline<SetMessageHook3>(pattern.get(3).get<void>(0), pattern.get(3).get<void>(6));

}

void InitPCSX2()
{
    //VCS();
    LCS();
}