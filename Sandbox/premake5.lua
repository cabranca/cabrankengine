project "Sandbox"
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

        postbuildcommands
        {
            'xcopy /Y /Q /E /I "%{prj.location}\\assets" "%{cfg.targetdir}\\assets"',
            'if exist "%{prj.location}\\config.json" copy /Y "%{prj.location}\\config.json" "%{cfg.targetdir}\\config.json"'
        }

    local renderer = _OPTIONS["renderer"]
    local linuxRenderer = renderer or "vulkan"

    filter "system:linux"
        systemversion "latest"
        pic "on"

        links { "X11", "Xrandr", "Xi", "Xxf86vm", "Xcursor", "pthread", "dl", "GL", "z", "glad" }

    if linuxRenderer == "vulkan" then
        filter "system:linux"
            libdirs({ "%{LibDir.vulkan}" })
            links { "slang", "slang-compiler" }
            defines({ "CBK_RENDERER_VULKAN" })
    else
        filter "system:linux"
            defines({ "CBK_RENDERER_OPENGL" })
    end
        postbuildcommands
        {
            'cp -ru %{prj.location}/assets/ %{cfg.targetdir}/',
            'cp -u %{prj.location}/config.json %{cfg.targetdir}/config.json 2>/dev/null || true'
        }

    filter "system:macosx"
        systemversion "12.0"
        pic "On"

        links { "Cocoa.framework", "IOKit.framework", "Foundation.framework", "Metal.framework", "QuartzCore.framework", "z" }

        postbuildcommands 
        {
            'cp -R %{prj.location}/assets %{cfg.targetdir}/',
            'cp -f %{prj.location}/config.json %{cfg.targetdir}/config.json || true'
        }

    filter "configurations:Debug"
        defines "CBK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "CBK_RELEASE"
        runtime "Release"
        optimize "on"
