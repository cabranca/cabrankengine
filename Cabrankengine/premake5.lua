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
		"%{IncludeDir.glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.json}",
		"%{IncludeDir.FreeType}",
		"%{IncludeDir.lz4}",
	})

	links({ "Common", "GLFW", "ImGui", "FreeType" })

	local renderer = _OPTIONS["renderer"]
	local linuxRenderer = renderer or "vulkan"
	local windowsRenderer = renderer or "opengl"

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
		})

		externalincludedirs({ "%{IncludeDir.vulkan}" })
		defines({ "GLFW_INCLUDE_NONE" })

	if windowsRenderer == "vulkan" then
		filter("system:windows")
			files({
				"src/Platform/Vulkan/**.h",
				"src/Platform/Vulkan/**.cpp",
			})
			defines({ "CBK_RENDERER_VULKAN" })
			libdirs({ "%{LibDir.vulkan}" })
			links({ "glad", "vulkan-1.lib" })
	else
		filter("system:windows")
			files({
				"src/Platform/OpenGL/**.h",
				"src/Platform/OpenGL/**.cpp",
			})
			defines({ "CBK_RENDERER_OPENGL" })
			links({ "opengl32.lib", "glad" })
	end

	filter("system:linux")
		systemversion("latest")
		pic("on")

		pchheader("pch.h")
		pchsource("src/pch.cpp")

		files({
			"src/Platform/Linux/**.h",
			"src/Platform/Linux/**.cpp",
		})

		includedirs({ "src" })
		links({ "X11", "Xrandr", "Xi", "Xxf86vm", "Xcursor", "pthread", "dl", "GL", "glad" })

	if linuxRenderer == "vulkan" then
		filter("system:linux")
			files({
				"src/Platform/Vulkan/**.h",
				"src/Platform/Vulkan/**.cpp",
			})
			defines({ "CBK_RENDERER_VULKAN" })
			externalincludedirs({ "%{IncludeDir.vulkan}" })
			libdirs({ "%{LibDir.vulkan}" })
			links({ "slang", "slang-compiler" })
	else
		filter("system:linux")
			files({
				"src/Platform/OpenGL/**.h",
				"src/Platform/OpenGL/**.cpp",
			})
			defines({ "CBK_RENDERER_OPENGL" })
	end

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
