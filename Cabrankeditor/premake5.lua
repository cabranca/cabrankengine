project "Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files {"src/**.h", "src/**.cpp"}

    externalincludedirs
    {
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.spdlog}", "%{IncludeDir.Common}",
        "%{wks.location}/Cabrankengine/src",
    }

    links 
    {
        "Cabrankengine", "Common", "FreeType", "GLFW", "ImGui"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }

        links { "glad" }

    filter "system:linux"
        systemversion "latest"
        pic "on"

        links { "X11", "Xrandr", "Xi", "Xxf86vm", "Xcursor", "pthread", "dl", "GL", "z", "glad" }

    filter "system:macosx"
        systemversion "12.0"
        pic "On"

        links { "Cocoa.framework", "IOKit.framework", "Foundation.framework", "Metal.framework", "QuartzCore.framework", "z" }

    filter "configurations:Debug"
        defines "CBK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "CBK_RELEASE"
        runtime "Release"
        optimize "on"
