workspace "workspace"
    configurations { "Debug", "Release" }

project "example1"
    kind "ConsoleApp"
    location "buildFiles"
    language "C++"
    cppdialect "C++17"
    files { "**.hpp", "**.cpp" }

    filter { "configurations:Debug" }
        defines { "DEBUG" }
        flags { "Symbols" }

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"