#include <pch.h>
#include "Renderer.h"

#include <Cabrankengine/Scene/DefaultLibrary.h>

#include "Materials/Material.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "TextRenderer.h"

namespace cbk::rendering {

	using namespace math;
	using namespace scene;

	

	void Renderer::init(const Window& window) {
		CBK_PROFILE_FUNCTION();

		RenderCommand::init(window);
		//DefaultLibrary::init();
		//Renderer2D::init();
		//TextRenderer::init();
	}

	void Renderer::shutdown() {
		//DefaultLibrary::shutdown();
		ShaderLibrary::shutdown(); // This should be in the same class that initialize it.
		//TextRenderer::shutdown();
		//Renderer2D::shutdown();

		RenderCommand::shutdown();
	}

	void Renderer::beginScene(const SceneData& sceneData) {
		RenderCommand::beginScene(sceneData);
	}

	void Renderer::endScene() {}

	void Renderer::submit(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const Mat4& transform) {
		RenderCommand::drawIndexed(material, desc, transform, desc->getIndexCount());
	}

	void Renderer::onWindowResize(uint32_t width, uint32_t height) {
		RenderCommand::setViewport(0, 0, width, height);
	}
} // namespace cbk::rendering
