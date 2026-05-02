workspace "Cabrankengine"
    architecture "x64"
    startproject "Sandbox"

    configurations {"Debug", "Release"}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["assimp"] = "%{wks.location}/CBKAssetConverter/vendor/assimp/include"
IncludeDir["Catch2"] = "%{wks.location}/Cabrankengine/vendor/Catch2"
IncludeDir["FreeType"] = "%{wks.location}/Cabrankengine/vendor/freetype/include"
IncludeDir["GLFW"] = "%{wks.location}/Cabrankengine/vendor/GLFW/include"
IncludeDir["glad"] = "%{wks.location}/Cabrankengine/vendor/glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Cabrankengine/vendor/imgui"
IncludeDir["json"] = "%{wks.location}/Cabrankengine/vendor/json/include"
IncludeDir["lz4"] = "%{wks.location}/Common/vendor/lz4"
IncludeDir["Metal"] = "%{wks.location}/Cabrankengine/vendor/metal-cpp"
IncludeDir["spdlog"] = "%{wks.location}/Common/vendor/spdlog/include"
IncludeDir["Common"] = "%{wks.location}/Common/src"
IncludeDir["stb_image"] = "%{wks.location}/CBKAssetConverter/vendor/stb_image"

include "Common"
include "Cabrankengine"
include "Cabrankengine/vendor/freetype"
include "Cabrankengine/vendor/GLFW"
include "Cabrankengine/vendor/glad"
include "Cabrankengine/vendor/imgui"
include "Sandbox"
include "CBKAssetConverter/vendor/assimp"
include "CBKAssetConverter"
include "UnitTests"

filter "system:macosx"
    systemversion "latest" -- Usa el SDK que tengas instalado (el 13 o 14)
    
    -- FORZAR COMPATIBILIDAD CON MONTEREY (12.0)
    -- Esto le dice al compilador y al linker: "Aunque tengas el SDK nuevo, generá código para 12.0"
    buildoptions { "-mmacosx-version-min=12.0" }
    linkoptions  { "-mmacosx-version-min=12.0" }
