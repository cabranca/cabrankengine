#include <pch.h>
#ifdef CBK_RENDERER_VULKAN
// Volk lives at <volk/volk.h> in this project; include it first so imgui sees
// the function pointers, then suppress imgui's own Vulkan header pull-in.
#include <volk/volk.h>
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <backends/imgui_impl_vulkan.cpp>
#else
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.cpp>
#endif
#include <backends/imgui_impl_glfw.cpp>
