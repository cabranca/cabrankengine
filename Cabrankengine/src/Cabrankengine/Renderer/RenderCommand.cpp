#include <pch.h>
#include "RenderCommand.h"

#ifdef CBK_RENDERER_OPENGL
#include <Platform/OpenGL/OpenGLRendererAPI.h>

namespace cbk::rendering {

	RendererAPI* RenderCommand::s_RendererAPI = new platform::opengl::OpenGLRendererAPI;
}
#endif

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalRendererAPI.h>

namespace cbk::rendering {

	RendererAPI* RenderCommand::s_RendererAPI = new platform::metal::MetalRendererAPI;
}
#endif

#ifdef CBK_RENDERER_VULKAN
#include <Platform/Vulkan/VulkanRendererAPI.h>

namespace cbk::rendering {

    RendererAPI* RenderCommand::s_RendererAPI = new platform::vk::VulkanRendererAPI;
}
#endif
