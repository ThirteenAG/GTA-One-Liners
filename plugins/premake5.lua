workspace "GTA-One-Liners-Recorder"
   configurations { "Release", "Debug" }
   architecture "x86"
   location "build"
   buildoptions {"-std:c++latest"}
   kind "SharedLib"
   language "C++"
   targetextension ".asi"
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.dll\"" }
   defines { "rsc_FileDescription=\"GTA-One-Liners-Recorder\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/GTA-One-Liners/\"" }

   includedirs { "source" }
   includedirs { "external/injector/include" }
   includedirs { "external/hooking" }
   files { "source/resources/Versioninfo.rc" }
   files { "source/dllmain.cpp" }
   files { "source/stories.cpp" }
   files { "source/episodes.cpp" }
   files { "source/meowhunt.cpp" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   
   characterset ("Unicode")
   
   pbcommands = { 
      "setlocal EnableDelayedExpansion",
      --"set \"path=" .. (gamepath) .. "\"",
      "set file=$(TargetPath)",
      "FOR %%i IN (\"%file%\") DO (",
      "set filename=%%~ni",
      "set fileextension=%%~xi",
      "set target=!path!!filename!!fileextension!",
      "if exist \"!target!\" copy /y \"!file!\" \"!target!\"",
      ")" }

   function setpaths (gamepath, exepath, scriptspath)
      scriptspath = scriptspath or "scripts/"
      if (gamepath) then
         cmdcopy = { "set \"path=" .. gamepath .. scriptspath .. "\"" }
         table.insert(cmdcopy, pbcommands)
         postbuildcommands (cmdcopy)
         debugdir (gamepath)
         if (exepath) then
            debugcommand (gamepath .. exepath)
            dir, file = exepath:match'(.*/)(.*)'
            debugdir (gamepath .. (dir or ""))
         end
      end
      targetdir ("data")
   end
   
   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      staticruntime "On"
	  
project "GTA-One-Liners-Recorder"
   setpaths("Z:/WFP/Games/Grand Theft Auto/GTA San Andreas/", "gta_sa.exe")
project "GTA-One-Liners-Recorder-PCSX2"
   setpaths("Z:/WFP/Games/PCSX2/", "pcsx2.exe")
project "GTA-One-Liners-Renamer"
   kind "ConsoleApp"
   targetextension ".exe"
   targetdir ("data")
   
   
workspace "GTA-One-Liners-x64"
   configurations { "Release", "Debug" }
   architecture "x64"
   location "build"
   buildoptions {"-std:c++latest"}
   kind "SharedLib"
   language "C++"
   targetextension ".asi"
   characterset ("Unicode")
   
   defines { "rsc_CompanyName=\"ThirteenAG\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.dll\"" }
   defines { "rsc_FileDescription=\"GTA-One-Liners-Recorder\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/GTA-One-Liners/\"" }
   
   includedirs { "source" }
   includedirs { "external/injector/include" }
   includedirs { "external/hooking" }
   files { "source/resources/Versioninfo.rc" }
   files { "source/v.cpp" }
   files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
   
project "GTA-One-Liners-Recorder-V"
   setpaths("X:/Games/Grand Theft Auto V/", "GTAVLauncher.exe")