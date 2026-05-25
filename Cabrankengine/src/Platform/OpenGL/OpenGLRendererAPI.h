#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

namespace cbk::platform::opengl {

	class OpenGLRendererAPI : public rendering::RendererAPI {
	  public:
		// Initializes the renderer API. This method should be called before any rendering operations.
		virtual void init() override;

		void shutdown() override;

		// Sets the color used to clear the screen.
		virtual void setClearColor(const math::Vector4& color) override;

		// Clears the screen with the previously set clear color.
		virtual void clear() override;

		void beginFrame() override;

		// Draws the vertex array vertices in order
		virtual void draw(const Ref<rendering::GeometryDescriptor>& vertexArray) override;

		// Draws the indexed vertices from the vertex array.
		virtual void drawIndexed(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& vertexArray,
		                         const math::Mat4& transform, uint32_t indexCount = 0) override;

		virtual void endFrame() override;

		// Sets the viewport dimensions for rendering.
		virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		// Returns the current rendering API.
		static API getAPI() {
			return API::OpenGL;
		}
	};
} // namespace cbk::platform::opengl
