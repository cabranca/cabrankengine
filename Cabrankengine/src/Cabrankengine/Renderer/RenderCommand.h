#pragma once

#include <Cabrankengine/Math/MatrixFactory.h>

#include "RendererAPI.h"

namespace cbk::rendering {

	// RenderCommand is a class that provides a static interface for rendering commands.
	// Now it seems to just forward calls to the RendererAPI, but in the future the idea
	// is to be able to queue commands and execute them in another thread.
	class RenderCommand {
	  public:
		// Initializes the RendererAPI. This should be called once at the start of the application.
		static void init() {
			s_RendererAPI->init();
		}

		static void shutdown() {
			s_RendererAPI->shutdown();
		}

		// Sets the color used to clear the screen.
		static void setClearColor(const math::Vector4& color) {
			s_RendererAPI->setClearColor(color);
		}

		// Clears the screen with the previously set clear color.
		static void clear() {
			s_RendererAPI->clear();
		}

		static void beginFrame() {
			s_RendererAPI->beginFrame();
		}

		// Draws the vertex array vertices in order
		static void draw(const Ref<GeometryDescriptor>& desc) {
			s_RendererAPI->draw(desc);
		}

		// Draws the indexed vertices from the vertex array. Transform defaults to identity for
		// batched 2D / text submissions whose vertices are already in world space.
		static void drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc,
		                        const math::Mat4& transform = math::identityMat(), uint32_t indexCount = 0) {
			s_RendererAPI->drawIndexed(material, desc, transform, indexCount);
		}

		static void endFrame() {
			s_RendererAPI->endFrame();
		}

		static void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
			s_RendererAPI->setViewport(x, y, width, height);
		}

		static RendererAPI* getRendererAPI() { return s_RendererAPI; }

	  private:
		static RendererAPI* s_RendererAPI; // This is a pointer to the RendererAPI instance that will be used for rendering commands.
	};
} // namespace cbk::rendering
