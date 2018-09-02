-- premake5.lua
workspace "V3D"
   configurations { "Debug", "Release" }

project "V3D"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"

   files { "**.h", "**.cpp" }

   filter "configurations:Debug"
      defines { "SYS_DEBUG_ODS" }
      symbols "On"

      links { "GL", "SDL2","SDL2_ttf" }

   filter "configurations:Release"
      defines { "SYS_RELEASE_ODS" }
      optimize "On"

      links { "GL", "SDL2","SDL2_ttf" }
