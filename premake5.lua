workspace "Kingdoms"
    architecture "x64"
    --toolset "msc-ClangCL"
    toolset "msc"

    configurations
    {
        "Debug",
        "Release"
    }

    startproject "Game"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

newoption
{
    trigger = "graphics",
    value = "OPENGL_VERSION",
    description = "version of OpenGL to build raylib against",
    allowed = {
	    { "opengl11", "OpenGL 1.1"},
	    { "opengl21", "OpenGL 2.1"},
	    { "opengl33", "OpenGL 3.3"},
	    { "opengl43", "OpenGL 4.3"}
    },
    default = "opengl33"
}

filter {}

include "Game/vendor/raylib_premake5.lua"

local directories = {
    "./",
    "Game/",
    "Game/vendor/"
}

local function DeleteVSFiles(path)
    os.remove(path .. "*.sln")
    os.remove(path .. "*.vcxproj")
    os.remove(path .. "*.vcxproj.filters")
    os.remove(path .. "*.vcxproj.user")
    print("Deleting VS files for " .. path)
end

newaction
{
    trigger = "clean",
    description = "Remove all binaries, int-binaries, vs files",
    execute = function()
        os.rmdir("./bin")
        print("Successfully removed binaries")
        os.rmdir("./bin-int")
        print("Successfully removed intermediate binaries")
        os.rmdir("./.vs")

        for _, v in pairs(directories) do
            DeleteVSFiles(v)
        end

        print("Successfully removed vs project files")
        print("Done!")
    end
}

filter {}

project "Game"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "Game/src/main.cpp",
        "Game/src/GameState.cpp",
        "Game/vendor/zpl/zpl.h"
    }

    defines
    {
        "SCAL_BUILD_DLL",
    }

    includedirs
    {
        "Game/src",
        "Game/vendor",
        "D:/dev-libs/sx/include"
    }

    libdirs
    {
        "%{wks.location}/bin/" .. outputdir .. "/raylib/",
        "D:/dev-libs/sx/build/Debug"
    }

    links
    {
        "sx"
    }

    dependson
    {
        "raylib"
    }

    filter "toolset:msc-ClangCL"
        buildoptions
        { 
            "-Wc++17-compat", "-Weverything", "-Wno-c++98-compat-pedantic",
            "-Wno-old-style-cast", "-Wno-extra-semi-stmt"
        }

    filter "configurations:Debug"
        defines "SCAL_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SCAL_RELEASE"
        runtime "Release"
        optimize "on"

    filter "system:Windows"
        defines "SCAL_PLATFORM_WINDOWS"
        systemversion "latest"
        links { "raylib.lib" }

    filter "system:Unix"
        defines "SCAL_PLATFORM_LINUX"
        links { "raylib.o" }

    filter {}
