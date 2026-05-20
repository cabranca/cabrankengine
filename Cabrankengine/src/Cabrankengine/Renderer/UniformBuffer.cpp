#include <pch.h>
#include "UniformBuffer.h"

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLUniformBuffer.h>
#endif

#ifdef CBK_RENDERER_VULKAN
    #include <Platform/Vulkan/VulkanUniformBuffer.h>
#endif

namespace cbk::rendering {

	Ref<UniformBuffer> UniformBuffer::create(uint32_t size, uint32_t binding) {
#ifdef CBK_RENDERER_OPENGL
		return std::make_shared<platform::opengl::OpenGLUniformBuffer>(size, binding);
#elif defined(CBK_RENDERER_METAL)
		return nullptr; // TODO: Metal UniformBuffer not yet implemented
#elif defined(CBK_RENDERER_VULKAN)
        return std::make_shared<platform::vk::VulkanUniformBuffer>(size, binding);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering