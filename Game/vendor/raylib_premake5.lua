project "raylib"
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    
    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "raylib/src/*.c",
        "raylib/src/raylib.h"
    }

    includedirs
    {
        "raylib/src",
        "raylib/src/external",
        "raylib/src/external/glfw/include"
    }

    defines
    {
        "BUILD_LIBTYPE_SHARED",
        "_WINSOCK_DEPRECATED_NO_WARNINGS",
        "_CRT_SECURE_NO_WARNINGS"
    }
    
    filter "system:windows"
        defines{ "_WIN32", "PLATFORM_DESKTOP"}
        links { "winmm", "kernel32", "opengl32", "gdi32", "user32" }

    filter "system:linux"
        defines { "PLATFORM_DESKTOP" }
        links { "pthread", "GL", "m", "dl", "rt", "X11" }

    filter {"options:graphics=opengl43"}
        defines{"GRAPHICS_API_OPENGL_43"}

    filter {"options:graphics=opengl33"}
        defines{"GRAPHICS_API_OPENGL_33"}

    filter {"options:graphics=opengl21"}
        defines{"GRAPHICS_API_OPENGL_21"}

    filter {"options:graphics=opengl11"}
        defines{"GRAPHICS_API_OPENGL_11"}

    filter {"system:macosx"}
        disablewarnings {"deprecated-declarations"}

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        runtime "Release"

    filter { "platforms:x64" }
        architecture "x86_64"

    filter {}

    postbuildcommands
    {
       ("{COPYFILE} %{wks.location}bin/" .. outputdir .. "/%{prj.name}/raylib.dll "
            .. "%{wks.location}bin/" .. outputdir .. "/%{prj.name}/../Engine/raylib.dll");

        ("{COPYFILE} %{wks.location}bin/" .. outputdir .. "/%{prj.name}/raylib.dll "
            .. "%{wks.location}bin/" .. outputdir .. "/%{prj.name}/../Game/raylib.dll");

        ("echo Copying raylib.dll to Game file!");
    }
