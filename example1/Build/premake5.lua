workspace "workspace"
	configurations { "Debug", "Release" }

	project "example1"
		kind "ConsoleApp"
		language "C++"
		files { "../src/**.hpp", "../src/**.cpp" }
		cppdialect "C++17"

		filter { "configurations:Debug" }
			defines { "DEBUG" }
			flags { "Symbols" }


		filter { "configurations:Release" }
			defines { "NDEBUG" }
			optimize "Full"