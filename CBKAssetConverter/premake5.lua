project "CBKAssetConverter"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files { "src/**.cpp", "vendor/stb_image/stb_image.cpp", }
    externalincludedirs { "%{IncludeDir.assimp}", "%{IncludeDir.lz4}", "%{IncludeDir.stb_image}", "%{IncludeDir.spdlog}", "%{IncludeDir.Common}" }
    links { "Assimp", "Common" }

    filter "system:linux"
        systemversion "latest"
        pic "on"
        links { "z" }

    filter "system:macosx"
        links { "z" }

    filter "system:windows"
        buildoptions { "/utf-8" }

    filter "configurations:Debug"
        defines "CBK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "CBK_RELEASE"
        runtime "Release"
        optimize "on"
