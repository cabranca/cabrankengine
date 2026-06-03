#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

namespace MTL {
	class RenderCommandEncoder;
	class CommandBuffer;
} // namespace MTL

namespace cbk::platform::metal {

	class MetalShader; // Forward declaration

	class MetalRendererAPI : public rendering::RendererAPI {
	  public:
		// Initializes the renderer API. This method should be called before any rendering operations.
		void init() override;

		void shutdown() override;

		// Sets the color used to clear the screen.
		void setClearColor(const math::Vector4& color) override;

		// Clears the screen with the previously set clear color.
		void clear() override;

		void beginFrame() override;

		// Draws the vertex array vertices in order
		void draw(const Ref<rendering::GeometryDescriptor>& desc) override;

		// Draws the indexed vertices from the vertex array.
		void drawIndexed(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& vertexArray, const math::Mat4& transform,
			                         uint32_t indexCount = 0) override;

		void endFrame() override;

		// Sets the viewport dimensions for rendering.
		void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		// Returns the current rendering API.
		static API getAPI() {
			return API::Metal;
		}

		// Sets the currently active Metal shader for the next draw call.
		static void SetCurrentShader(MetalShader* shader);

		// Returns the active render command encoder (used by textures to bind themselves).
		static MTL::RenderCommandEncoder* GetActiveEncoder() { return s_ActiveEncoder; }

	  private:
		math::Vector4 m_ClearColor = { 0.f, 0.f, 0.f, 1.f };

		// Per-frame state (static so other Metal subsystems can access them)
        MTL::CommandBuffer* m_ActiveCommandBuffer = nullptr;
        inline static MTL::RenderCommandEncoder* s_ActiveEncoder = nullptr;
		inline static MetalShader* s_CurrentShader = nullptr;
	};
} // namespace cbk::platform::metal
