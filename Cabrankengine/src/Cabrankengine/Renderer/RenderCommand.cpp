#include <pch.h>
#include "RenderCommand.h"

#ifdef CBK_RENDERER_OPENGL
#include <Platform/OpenGL/OpenGLRendererAPI.h>

namespace cbk::rendering {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = std::make_unique<platform::opengl::OpenGLRendererAPI>();
}
#endif

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalRendererAPI.h>

namespace cbk::rendering {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = std::make_unique<platform::metal::MetalRendererAPI>();
}
#endif

#ifdef CBK_RENDERER_VULKAN
#include <Platform/Vulkan/VulkanRendererAPI.h>

namespace cbk::rendering {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = std::make_unique<platform::vk::VulkanRendererAPI>();
}
#endif

namespace cbk::rendering {

	void RenderCommand::init() {
		s_RendererAPI->init();
	}

	void RenderCommand::shutdown() {
		s_RendererAPI->shutdown();
	}

	void RenderCommand::setClearColor(const math::Vector4& color) {
		s_RendererAPI->setClearColor(color);
	}

	void RenderCommand::clear() {
		s_RendererAPI->clear();
	}

	void RenderCommand::beginFrame() {
		s_RendererAPI->beginFrame();
	}

	void RenderCommand::draw(const Ref<GeometryDescriptor>& desc) {
		s_RendererAPI->draw(desc);
	}

	void RenderCommand::drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform,
	                                uint32_t indexCount) {
		CBK_CORE_ASSERT(material, "RenderCommand::drawIndexed(): trying to render without material!");
		s_RendererAPI->drawIndexed(material, desc, transform, indexCount);
	}

	void RenderCommand::endFrame() {
		s_RendererAPI->endFrame();
	}

	void RenderCommand::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		s_RendererAPI->setViewport(x, y, width, height);
	}

	RendererAPI* RenderCommand::getRendererAPI() {
		return s_RendererAPI.get();
	}
} // namespace cbk::rendering
