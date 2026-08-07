project("Cabrankengine")
	kind("StaticLib")
	language("C++")
	cppdialect("C++23")
	staticruntime("on")

	targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files({
		"src/Cabrankengine/**.h",
		"src/Cabrankengine/**.cpp",
	})

	externalincludedirs({
		"src",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.Common}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.json}",
		"%{IncludeDir.FreeType}",
		"%{IncludeDir.lz4}",
	})

	links({ "Common", "GLFW", "ImGui", "FreeType" })

	filter("system:windows")
		systemversion("latest")
		buildoptions({ "/utf-8" })

		pchheader("pch.h")
		pchsource("src/pch.cpp")
		forceincludes({ "pch.h" })

		files({
			"src/pch.h",
			"src/pch.cpp",
			"src/Platform/Windows/**.h",
			"src/Platform/Windows/**.cpp",
			"src/Platform/Vulkan/**.h",
			"src/Platform/Vulkan/**.cpp",
		})

		externalincludedirs({ "%{IncludeDir.vulkan}" })
		defines({ "GLFW_INCLUDE_NONE", "CBK_RENDERER_VULKAN" })
		libdirs({ "%{LibDir.vulkan}" })
		links({ "vulkan-1.lib" })

	filter("system:linux")
		systemversion("latest")
		pic("on")

		pchheader("pch.h")
		pchsource("src/pch.cpp")

		files({
			"src/Platform/Linux/**.h",
			"src/Platform/Linux/**.cpp",
			"src/Platform/Vulkan/**.h",
			"src/Platform/Vulkan/**.cpp",
		})

		includedirs({ "src" })
		links({ "X11", "Xrandr", "Xi", "Xxf86vm", "Xcursor", "pthread", "dl", "slang", "slang-compiler" })
		defines({ "CBK_RENDERER_VULKAN" })
		externalincludedirs({ "%{IncludeDir.vulkan}" })
		libdirs({ "%{LibDir.vulkan}" })

	filter("system:macosx")
		systemversion("12.0")
		pic("On")

		pchheader("src/pch.h")
		pchsource("src/pch.cpp")

		defines({ "IMGUI_IMPL_METAL_CPP" })

		files({
		"src/Platform/MacOS/**.h",
		"src/Platform/MacOS/**.cpp",
		"src/Platform/Metal/**.h",
		"src/Platform/Metal/**.cpp",
		"src/Platform/Metal/**.mm",
		"vendor/imgui/backends/imgui_impl_metal.mm",
		-- ImGuiBuild.cpp (removed below) pulls in the GLFW backend on the other
		-- platforms; on Metal we compile the Metal + GLFW backends directly.
		"vendor/imgui/backends/imgui_impl_glfw.cpp",
		})

		removefiles({ "src/Cabrankengine/ImGui/ImGuiBuild.cpp" })

		externalincludedirs({ "%{IncludeDir.Metal}" })

		links({ "Cocoa.framework", "Foundation.framework", "Metal.framework", "QuartzCore.framework" })

	filter("configurations:Debug")
		defines("CBK_DEBUG")
		runtime("Debug")
		symbols("on")

	filter("configurations:Release")
		defines("CBK_RELEASE")
		runtime("Release")
		optimize("on")

	filter("files:**.mm")
		enablepch("off")
