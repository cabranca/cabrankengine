workspace("Cabrankengine")
	startproject("Sandbox")

	filter("system:windows or system:linux")
		architecture("x64")
	filter({})

	configurations({ "Debug", "Release" })

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	-- Vulkan SDK location, taken from the VULKAN_SDK environment variable (Linux/Windows only)
	local vulkan_sdk = os.getenv("VULKAN_SDK")
	local needsVulkanSdk = (_OPTIONS["renderer"] == "vulkan") or
	                       (_OPTIONS["renderer"] == nil and os.host() == "linux")
	if not vulkan_sdk then
		error("VULKAN_SDK environment variable is not set. Install the Vulkan SDK and set VULKAN_SDK.")
	end

	-- Include directories relative to root folder (solution directory)
	IncludeDir = {}
	IncludeDir["assimp"] = "%{wks.location}/CBKAssetConverter/vendor/assimp/include"
	IncludeDir["Catch2"] = "%{wks.location}/Cabrankengine/vendor/Catch2"
	IncludeDir["FreeType"] = "%{wks.location}/Cabrankengine/vendor/freetype/include"
	IncludeDir["GLFW"] = "%{wks.location}/Cabrankengine/vendor/GLFW/include"
	IncludeDir["ImGui"] = "%{wks.location}/Cabrankengine/vendor/imgui"
	IncludeDir["json"] = "%{wks.location}/Cabrankengine/vendor/json/include"
	IncludeDir["lz4"] = "%{wks.location}/Common/vendor/lz4"
	IncludeDir["Metal"] = "%{wks.location}/Cabrankengine/vendor/metal-cpp"
	IncludeDir["spdlog"] = "%{wks.location}/Common/vendor/spdlog/include"
	IncludeDir["Common"] = "%{wks.location}/Common/src"
	IncludeDir["stb_image"] = "%{wks.location}/CBKAssetConverter/vendor/stb_image"
	IncludeDir["vulkan"] = vulkan_sdk .. "/include"
	
	-- Library directories
	LibDir = {}
	LibDir["vulkan"] = vulkan_sdk .. "/lib"


	include("Common")
	include("Cabrankengine")
	include("Cabrankengine/vendor/freetype")
	include("Cabrankengine/vendor/GLFW")
	include("Cabrankengine/vendor/imgui")
	include("Sandbox")
	include("CBKAssetConverter/vendor/assimp")
	include("CBKAssetConverter")
	include("UnitTests")


