#pragma once

#include <Common/Math/MatrixFactory.h>

#include "RendererAPI.h"

namespace cbk::rendering {

	// RenderCommand is a class that provides a static interface for rendering commands.
	// Now it seems to just forward calls to the RendererAPI, but in the future the idea
	// is to be able to queue commands and execute them in another thread.
	class RenderCommand {
	  public:
		// Initializes the RendererAPI. This should be called once at the start of the application.
		static void init();

		static void shutdown();

		// Sets the color used to clear the screen.
		static void setClearColor(const math::Vector4& color);

		static void beginFrame();

		// Draws the vertex array vertices in order
		static void draw(const Ref<GeometryDescriptor>& desc);

		// Draws the indexed vertices from the vertex array. Transform defaults to identity for
		// batched 2D / text submissions whose vertices are already in world space.
		static void drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc,
		                        const math::Mat4& transform = math::identityMat(), uint32_t indexCount = 0);

		static void endFrame();

		static void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		static RendererAPI* getRendererAPI();

	  private:
		static Scope<RendererAPI> s_RendererAPI; // This is a pointer to the RendererAPI instance that will be used for rendering commands.
	};
} // namespace cbk::rendering
