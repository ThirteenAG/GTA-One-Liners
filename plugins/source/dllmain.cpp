#include <windows.h>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <regex>

#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"

#include <locale>
#include <codecvt>

auto gvm = injector::address_manager::singleton();
std::wstring DefaultPathW;
std::vector<DWORD> Keys;
std::ofstream logfileA;
std::wofstream logfileW;
size_t bufSize = 400;

bool isRecording = false;
char* m_Message;
uint8_t* bDisplayHud;
uint8_t* bDisplayRadar;

bool isPCSX2 = false;
bool isManhunt = false;
bool isBully = false;
bool isMaxPayne3 = true;

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
    if (gvm.IsSA())
        DefaultPathW += L"\\Grand Theft Auto  San Andreas";
    else if (gvm.IsVC())
        DefaultPathW += L"\\Grand Theft Auto Vice City";
    else if (gvm.IsIII())
        DefaultPathW += L"\\Grand Theft Auto 3";
    else if (gvm.IsIV())
        DefaultPathW += L"\\Grand Theft Auto 4";
    else if (gvm.IsEFLC())
        DefaultPathW += L"\\Grand Theft Auto EFLC"; ///???????????
    else if (isPCSX2)
        DefaultPathW += L"\\PCSX2";
    else if (isManhunt)
        DefaultPathW += L"\\Manhunt";
    else if (isBully)
        DefaultPathW += L"\\Grand Theft Auto  San Andreas";
    else if (isMaxPayne3)
        DefaultPathW += L"\\Max Payne 3";

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
    if (!std::string_view(m_Message).empty())
    {
        logfileA << GetLatestFileName() << " // " << m_Message << std::endl;
    }
}

void LogRecordingW()
{
    auto s = std::wstring((wchar_t*)m_Message);
    if (!s.empty())
    {
        size_t pos;
        while ((pos = s.find(L"  ")) != std::wstring::npos)
            s = s.replace(pos, 2, L" ");

        logfileW << GetLatestFileName() << L" // " << s << std::endl;
    }
}

void ToggleRecording()
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
}

void __cdecl SetMessageW(wchar_t *str)
{
    auto m_wMessage = (wchar_t*)m_Message;
    if (str[0] == 0 || (wcscmp(str, m_wMessage) != 0))
    {
        if (isRecording)
        {
            ToggleRecording();
            Sleep(500);
            isRecording = false;
            LogRecordingW();
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

    if (isRecording)
    {
        *bDisplayHud = 0;
        if (gvm.IsVC())
            *bDisplayRadar = 2;
    }
    else
    {
        *bDisplayHud = 1;
        if (gvm.IsVC())
            *bDisplayRadar = 0;
    }

    if (str)
    {
        auto i = 0;
        while (1)
        {
            if (!str[i])
                break;

            m_wMessage[i] = str[i];

            ++i;

            if (i >= bufSize)
            {
                m_wMessage[i] = 0;
                return;
            }
        }
        m_wMessage[i] = 0;
    }
    else
    {
        m_wMessage[0] = 0;
    }
}

void __cdecl SetMessageA(char *str)
{
    if (m_Message[0] == 0 || (strcmp(str, m_Message) != 0))
    {
        if (isRecording)
        {
            ToggleRecording();
            Sleep(500);
            isRecording = false;
            LogRecordingA();
        }
    }
    else if (str[0] == '~' && str[1] == 'z' && str[2] == '~')
    {
        if (!isRecording)
        {
            ToggleRecording();
            isRecording = true;
        }
    }

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

    if (str)
    {
        auto i = 0;
        while (1)
        {
            if (!str[i])
                break;

            m_Message[i] = str[i];

            ++i;

            if (i >= bufSize)
            {
                m_Message[i] = 0;
                return;
            }
        }
        m_Message[i] = 0;
    }
    else
    {
        m_Message[0] = 0;
    }
}

void GetNVidiaSettings3()
{
    GetRegistryData(DefaultPathW, HKEY_CURRENT_USER, L"Software\\NVIDIA Corporation\\Global\\ShadowPlay\\NVSPCAPS", L"DefaultPathW");
    DefaultPathW += L"\\Max Payne 3";

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

void Init()
{
    if (!isMaxPayne3)
        GetNVidiaSettings();
    else
        GetNVidiaSettings3();

    if (gvm.IsSA())
        logfileA.open(DefaultPathW + L"\\log.txt", std::ios_base::app);
    else
        logfileW.open(DefaultPathW + L"\\log.txt", std::ios_base::app);

    if (isMaxPayne3)
    {
        logfileW.imbue(std::locale("en_US.UTF-8"));
    }

    if (gvm.IsIII())
    {
        m_Message = (char*)0x72E318;
        bDisplayHud = (uint8_t*)0x95CD89;
        bufSize = 256;

        injector::MakeCALL(0x407850, SetMessageW, true);
        injector::MakeCALL(0x5298E4, SetMessageW, true);
    }
    else
    {
        if (gvm.IsVC())
        {
            m_Message = (char*)0x814F28;
            bDisplayHud = (uint8_t*)0xA10B45;
            bDisplayRadar = (uint8_t*)0x869665;
            bufSize = 256;

            injector::MakeCALL(0x40EDE0, SetMessageW, true);
            injector::MakeCALL(0x584634, SetMessageW, true);
        }
        else
        {
            if (gvm.IsSA())
            {
                m_Message = (char*)0xBAB040;
                bDisplayHud = (uint8_t*)0xBA6769;
                bDisplayRadar = (uint8_t*)0xBA676C;
                bufSize = 400;

                injector::MakeCALL(0x69F09C, SetMessageA, true);
                injector::MakeCALL(0x4427EE, SetMessageA, true);
            }
            else
            {
                if (gvm.IsIV())
                {
                    bDisplayHud = (uint8_t*)injector::aslr_ptr(0x10FC0FC).get();
                    bDisplayRadar = (uint8_t*)injector::aslr_ptr(0x10FC10C).get();

                    void InitIV();
                    InitIV();
                }
                else
                {
                    if (gvm.IsEFLC())
                    {

                    }
                    else
                    {
                        if (isManhunt)
                        {
                            m_Message = (char*)0x7213B0;
                            bDisplayHud = (uint8_t*)0x7CF0A0;
                            bDisplayRadar = (uint8_t*)0x7CF0A0;

                            void InitMH();
                            InitMH();
                        }
                        else
                        {
                            if (isBully)
                            {
                                void InitBully();
                                InitBully();
                            }
                            else if (isMaxPayne3)
                            {
                                //bDisplayHud = (uint8_t*)injector::aslr_ptr(0x10FC0FC).get();
                                //bDisplayRadar = (uint8_t*)injector::aslr_ptr(0x10FC10C).get();

                                void InitMaxPayne3();
                                InitMaxPayne3();
                            }
                        }
                    }
                }
            }
        }
    }
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
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&Init, &hm);
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
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&Init, &hm);
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
        if (GetExeModuleName<std::string>() == "pcsx2.exe")
            isPCSX2 = true;

        Init();

        if (isPCSX2)
        {
            extern void InitPCSX2();
            std::thread t(InitPCSX2);
            t.detach();
        }
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}

uint32_t ParseText(std::map<std::string, std::string>& m, std::wstring fileName)
{
    m.clear();

    std::ifstream textFile(GetExeModulePath<std::wstring>() + fileName);
    if (textFile.is_open())
    {
        std::string line;
        while (getline(textFile, line))
        {
            auto delimiter = std::string("//  ");
            auto delimiterPos = line.find(delimiter);

            if (/*line[0] == '#' ||*/ line.empty() || delimiterPos == std::string::npos)
                continue;

            auto name = line.substr(0, min(min(delimiterPos, line.find("\t")), line.find(" ")));
            auto value = line.substr(delimiterPos + delimiter.length());
            m.emplace(value/*.substr(0, 50)*/, name);
            //std::cout << name << " " << value << '\n';
        }
    }
    else
    {
        std::cerr << "Couldn't open text file for reading.\n";
        return 0;
    }
    return 1;
}

void RenameFiles(std::map<std::string, std::string>& m, std::wstring folderName)
{
    std::ifstream logFile(DefaultPathW + folderName + L"\\log.txt");
    if (logFile.is_open())
    {
        std::string line;
        while (getline(logFile, line))
        {
            auto delimiter = std::string("// ");
            auto delimiterPos = line.find(delimiter);

            if (line[0] == '#' || line.empty() || delimiterPos == std::string::npos)
                continue;

            //if (folderName == L"\\Grand Theft Auto  San Andreas")
            //    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return c == '\''; }), line.end());

            auto name = line.substr(0, min(delimiterPos, line.find("\t")));
            auto value = line.substr(delimiterPos + delimiter.length())/*.substr(0, 50)*/;

            if (name.front() == '"')
            {
                name.erase(0, 1);
                name.erase(name.size() - 2);
            }

            if (/*m.find(value) == m.end() || */name == "")
                continue;

            //auto gxt = m.at(value);
            auto gxt = std::string();
            for (auto var : m)
            {
                if (var.first.substr(0, value.size()) == value)
                {
                    gxt = m.at(var.first);
                    break;
                }
            }

            if (gxt.empty())
                continue;

            std::filesystem::path p = DefaultPathW + folderName + L"\\";
            auto oldName = p / name;
            auto newName = p / (gxt); newName += ".mp4";

            if (std::filesystem::exists(newName))
                continue;

            if (std::filesystem::exists(oldName))
                std::filesystem::rename(oldName, newName);

            //std::cout << oldName.filename() << " renamed to" << newName.filename() << '\n';
        }
    }
    else
    {
        std::cerr << "Couldn't open log.txt file for reading.\n";
    }
}

int main()
{
    GetNVidiaSettings();

    std::map<std::string, std::string> text;

    //if (ParseText(text, L"\\GTASA.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto  San Andreas");

    //if (ParseText(text, L"\\GTAVC.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto Vice City");

    //if (ParseText(text, L"\\GTAIII.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto 3");

    //if (ParseText(text, L"\\GTALCS.text"))
    //    RenameFiles(text, L"\\PCSX2");

    //if (ParseText(text, L"\\GTAVCS.text"))
    //    RenameFiles(text, L"\\PCSX2");

    //if (ParseText(text, L"\\text\\TLAD.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto 4");

    //if (ParseText(text, L"\\text\\TBOGT.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto 4");
    //
    //if (ParseText(text, L"\\text\\GTAIV.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto 4");

    //if (ParseText(text, L"\\text\\GTAV.text"))
    //    RenameFiles(text, L"\\Grand Theft Auto V");

    //if (isManhunt && ParseText(text, L"\\text\\MANHUNT.text"))
    //    RenameFiles(text, L"");

    //if (ParseText(text, L"\\text\\MAXPAYNE3.text"))
    //    RenameFiles(text, L"");

    //GTAV
    /*
    if (ParseText(text, L"\\text\\GTAV.text"))
    {
        namespace fs = std::filesystem;
        fs::path someDir(DefaultPathW + L"\\Grand Theft Auto V");
        fs::directory_iterator end_iter;

        typedef std::multimap<fs::file_time_type, fs::path> result_set_t;
        result_set_t result_set;
        std::vector<fs::path> files;

        if (fs::exists(someDir) && fs::is_directory(someDir))
        {
            for (fs::directory_iterator dir_iter(someDir); dir_iter != end_iter; ++dir_iter)
            {
                if (fs::is_regular_file(dir_iter->status()))
                {
                    if (dir_iter->path().extension() == ".mp4")
                        result_set.insert(result_set_t::value_type(fs::last_write_time(dir_iter->path()), *dir_iter));
                }
            }
            for each (auto var in result_set)
                files.push_back(var.second);
        }

        CreateDirectory(std::wstring(DefaultPathW + L"\\Grand Theft Auto V\\out").c_str(), NULL);
        std::ifstream logFile(DefaultPathW + L"\\Grand Theft Auto V" + L"\\log.txt");
        if (logFile.is_open())
        {
            std::string line;
            size_t count = 0;
            while (getline(logFile, line))
            {
                auto delimiter = std::string("// ");
                auto delimiterPos = line.find(delimiter);

                if (line[0] == '#' || line.empty() || delimiterPos == std::string::npos)
                    continue;

                auto start = std::stoi(line.substr(0, min(delimiterPos, line.find("\t"))));
                auto duration = std::stoi(line.substr(0, min(delimiterPos, line.find("\t")))) - 350; // removing 350ms to remove overlap
                auto value = line.substr(delimiterPos + delimiter.length());
                auto gxt = text.at(value);
                //auto gxt = std::string();
                //for each (auto var in text)
                //{
                //    if (var.first.substr(0, value.size()) == value)
                //    {
                //        gxt = text.at(var.first);
                //        break;
                //    }
                //}

                if (gxt.empty() || count >= files.size())
                {
                    count++;
                    continue;
                }

                auto ffmpeg = std::wstring(L"D:\\Program Files\\FFmpeg\\ffmpeg.exe");
                auto source = files[count];
                auto dest = (source.parent_path() / "out" / gxt).replace_extension("mp4");

                auto format_duration = [](std::chrono::milliseconds ms) -> std::wstring
                {
                    using namespace std::chrono;
                    auto secs = duration_cast<seconds>(ms);
                    ms -= duration_cast<milliseconds>(secs);
                    auto mins = duration_cast<minutes>(secs);
                    secs -= duration_cast<seconds>(mins);
                    auto hour = duration_cast<hours>(mins);
                    mins -= duration_cast<minutes>(hour);

                    std::wstringstream ss;
                    ss << hour.count() << ":" << mins.count() << ":" << secs.count() << "." << ms.count();
                    return ss.str();
                };

                auto shield = [](std::wstring& s) ->std::wstring
                {
                    return L"\"" + s + L"\"";
                };

                std::wstring cmd = std::wstring(L"\"" + shield(ffmpeg) + L" -y -sseof -" + format_duration(std::chrono::milliseconds(start)) + L" -t " + format_duration(std::chrono::milliseconds(duration)) + + L" -i " + shield(source.wstring()) + L" -c copy " + shield(dest.wstring()));

                AllocConsole();
                if (_wsystem(cmd.c_str()) == 0)
                {
                    //MessageBox(0,0,0,0);
                }

                count++;
            }
        }
        else
        {
            std::cerr << "Couldn't open log.txt file for reading.\n";
        }



    }
    */

    if (isMaxPayne3)
    {
        if (ParseText(text, L"\\text\\MAXPAYNE3.text"))
        {
            namespace fs = std::filesystem;
            fs::path someDir(DefaultPathW);
            fs::directory_iterator end_iter;

            typedef std::multimap<fs::file_time_type, fs::path> result_set_t;
            result_set_t result_set;
            std::vector<fs::path> files;

            if (fs::exists(someDir) && fs::is_directory(someDir))
            {
                for (fs::directory_iterator dir_iter(someDir); dir_iter != end_iter; ++dir_iter)
                {
                    if (fs::is_regular_file(dir_iter->status()))
                    {
                        if (dir_iter->path().extension() == ".mp4")
                            result_set.insert(result_set_t::value_type(fs::last_write_time(dir_iter->path()), *dir_iter));
                    }
                }
                for (auto var : result_set)
                    files.push_back(var.second);
            }

            CreateDirectory(std::wstring(DefaultPathW + L"\\out").c_str(), NULL);
            std::ifstream logFile(DefaultPathW + L"\\log.txt");
            if (logFile.is_open())
            {
                std::string line;
                size_t count = 0;
                while (getline(logFile, line))
                {
                    auto delimiter = std::string("// ");
                    auto delimiterPos = line.find(delimiter);

                    if (line[0] == '#' || line.empty() || delimiterPos == std::string::npos)
                        continue;

                    auto start = std::stoi(line.substr(0, min(delimiterPos, line.find("\t"))));
                    auto duration = std::stoi(line.substr(0, min(delimiterPos, line.find("\t")))) - 350; // removing 350ms to remove overlap
                    auto value = line.substr(delimiterPos + delimiter.length());
                    auto gxt = text.at(value);
                    //auto gxt = std::string();
                    //for each (auto var in text)
                    //{
                    //    if (var.first.substr(0, value.size()) == value)
                    //    {
                    //        gxt = text.at(var.first);
                    //        break;
                    //    }
                    //}

                    if (gxt.empty() || count >= files.size())
                    {
                        count++;
                        continue;
                    }

                    auto ffmpeg = std::wstring(L"D:\\Program Files\\FFmpeg\\ffmpeg.exe");
                    auto source = files[count];
                    auto dest = (source.parent_path() / "out" / gxt).replace_extension("mp4");

                    auto format_duration = [](std::chrono::milliseconds ms) -> std::wstring
                    {
                        using namespace std::chrono;
                        auto secs = duration_cast<seconds>(ms);
                        ms -= duration_cast<milliseconds>(secs);
                        auto mins = duration_cast<minutes>(secs);
                        secs -= duration_cast<seconds>(mins);
                        auto hour = duration_cast<hours>(mins);
                        mins -= duration_cast<minutes>(hour);

                        std::wstringstream ss;
                        ss << hour.count() << ":" << mins.count() << ":" << secs.count() << "." << ms.count();
                        return ss.str();
                    };

                    auto shield = [](std::wstring s) ->std::wstring
                    {
                        return L"\"" + s + L"\"";
                    };

                    std::wstring cmd = std::wstring(L"\"" + shield(ffmpeg) + L" -y -sseof -" + format_duration(std::chrono::milliseconds(start)) + L" -t " + format_duration(std::chrono::milliseconds(duration)) + + L" -i " + shield(source.wstring()) + L" -c copy " + shield(dest.wstring()));

                    AllocConsole();
                    if (_wsystem(cmd.c_str()) == 0)
                    {
                        //MessageBox(0,0,0,0);
                    }

                    count++;
                }
            }
            else
            {
                std::cerr << "Couldn't open log.txt file for reading.\n";
            }
        }

    }

    if (isBully)
    {
        if (ParseText(text, L"\\text\\BULLY.text"))
        {
            namespace fs = std::filesystem;
            fs::path someDir(DefaultPathW);
            fs::directory_iterator end_iter;

            typedef std::multimap<fs::file_time_type, fs::path> result_set_t;
            result_set_t result_set;
            std::vector<fs::path> files;

            if (fs::exists(someDir) && fs::is_directory(someDir))
            {
                for (fs::directory_iterator dir_iter(someDir); dir_iter != end_iter; ++dir_iter)
                {
                    if (fs::is_regular_file(dir_iter->status()))
                    {
                        if (dir_iter->path().extension() == ".mp4")
                            result_set.insert(result_set_t::value_type(fs::last_write_time(dir_iter->path()), *dir_iter));
                    }
                }
                for (auto var : result_set)
                    files.push_back(var.second);
            }

            CreateDirectory(std::wstring(DefaultPathW + L"\\out").c_str(), NULL);
            std::ifstream logFile(DefaultPathW + L"\\log.txt");
            if (logFile.is_open())
            {
                std::string line;
                size_t count = 0;
                while (getline(logFile, line))
                {
                    auto delimiter = std::string("// ");
                    auto delimiterPos = line.find(delimiter);

                    if (line[0] == '#' || line.empty() || delimiterPos == std::string::npos)
                        continue;

                    auto start = std::stoi(line.substr(0, min(delimiterPos, line.find("\t"))));
                    auto duration = std::stoi(line.substr(0, min(delimiterPos, line.find("\t")))) - 350; // removing 350ms to remove overlap
                    auto value = line.substr(delimiterPos + delimiter.length());
                    auto gxt = text.at(value);
                    //auto gxt = std::string();
                    //for each (auto var in text)
                    //{
                    //    if (var.first.substr(0, value.size()) == value)
                    //    {
                    //        gxt = text.at(var.first);
                    //        break;
                    //    }
                    //}

                    if (gxt.empty() || count >= files.size())
                    {
                        count++;
                        continue;
                    }

                    auto ffmpeg = std::wstring(L"D:\\Program Files\\FFmpeg\\ffmpeg.exe");
                    auto source = files[count];
                    auto dest = (source.parent_path() / "out" / gxt).wstring() + L".mp4";

                    auto format_duration = [](std::chrono::milliseconds ms) -> std::wstring
                    {
                        using namespace std::chrono;
                        auto secs = duration_cast<seconds>(ms);
                        ms -= duration_cast<milliseconds>(secs);
                        auto mins = duration_cast<minutes>(secs);
                        secs -= duration_cast<seconds>(mins);
                        auto hour = duration_cast<hours>(mins);
                        mins -= duration_cast<minutes>(hour);

                        std::wstringstream ss;
                        ss << hour.count() << ":" << mins.count() << ":" << secs.count() << "." << ms.count();
                        return ss.str();
                    };

                    auto shield = [](std::wstring s) ->std::wstring
                    {
                        return L"\"" + s + L"\"";
                    };

                    std::wstring cmd = std::wstring(L"\"" + shield(ffmpeg) + L" -y -sseof -" + format_duration(std::chrono::milliseconds(start)) + L" -t " + format_duration(std::chrono::milliseconds(duration)) + +L" -i " + shield(source.wstring()) + L" -c copy " + shield(dest));

                    AllocConsole();
                    if (_wsystem(cmd.c_str()) == 0)
                    {
                        //MessageBox(0,0,0,0);
                    }

                    count++;
                }
            }
            else
            {
                std::cerr << "Couldn't open log.txt file for reading.\n";
            }



        }
    }

    return 0;
}