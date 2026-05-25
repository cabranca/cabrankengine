#include <pch.h>
#include "GraphicsContext.h"

#ifdef CBK_RENDERER_METAL
	#include <Platform/Metal/MetalContext.h>
#endif

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLContext.h>
#endif

#ifdef CBK_RENDERER_VULKAN
    #include <Platform/Vulkan/VulkanDeviceContext.h>
#endif

namespace cbk::rendering {

	Scope<GraphicsContext> GraphicsContext::create(GLFWwindow* windowHandle) {
#ifdef CBK_RENDERER_OPENGL
		return createScope<platform::opengl::OpenGLContext>(windowHandle);
#elif defined(CBK_RENDERER_METAL)
		return createScope<platform::metal::MetalContext>(windowHandle);
#elif defined(CBK_RENDERER_VULKAN)
        return createScope<platform::vk::VulkanDeviceContext>();
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering
