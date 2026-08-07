#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

namespace MTL {
	class RenderCommandEncoder;
	class CommandBuffer;
	class Texture;
	class RenderPassDescriptor;
} // namespace MTL
namespace CA {
	class MetalLayer;
	class MetalDrawable;
} // namespace CA

namespace cbk::platform::metal {

	class MetalRendererAPI : public rendering::RendererAPI {
	  public:
		// Initializes the renderer API. This method should be called before any rendering operations.
		void init() override;

		void shutdown() override;

		// Sets the color used to clear the screen.
		void setClearColor(const math::Vector4& color) override;

		void beginFrame() override;

		// Draws the vertex array vertices in order
		void draw(const Ref<rendering::GeometryDescriptor>& desc) override;

		// Draws the indexed vertices from the vertex array.
		void drawIndexed(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& vertexArray,
		                 const math::Mat4& transform, uint32_t indexCount = 0) override;

		void endScenePass() override;

		void endFrame() override;

		// Sets the viewport dimensions for rendering.
		void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		// Returns the current rendering API.
		[[nodiscard]] static API getAPI() {
			return API::Metal;
		}

		[[nodiscard]] uint64_t getFinalFrame() const override;

		// Per-frame handles exposed for ImGui's Metal backend (NewFrame / RenderDrawData).
		[[nodiscard]] MTL::RenderPassDescriptor* getRenderPassDescriptor() const;
		[[nodiscard]] MTL::CommandBuffer* getCommandBuffer() const;
		[[nodiscard]] MTL::RenderCommandEncoder* getCommandEncoder() const;

	  private:
		math::Vector4 m_ClearColor = { 0.f, 0.f, 0.f, 1.f };

		// Per-frame state, valid between beginFrame() and endFrame(). The encoder is
		// threaded explicitly into materials/geometry during recording; it is also
		// exposed (with the command buffer and pass descriptor) so the ImGui layer can
		// draw into the same in-flight pass.
		MTL::CommandBuffer* m_ActiveCommandBuffer = nullptr;
		MTL::RenderCommandEncoder* m_ActiveEncoder = nullptr;
		MTL::RenderPassDescriptor* m_RenderPassDescriptor = nullptr;

		CA::MetalLayer* m_Swapchain;
		CA::MetalDrawable* m_CurrentDrawable = nullptr;

		// Depth buffer for the main pass. Recreated when the drawable size changes,
		// cleared to 1.0 each frame and never stored (DontCare). Mirrors the Vulkan
		// backend's per-swapchain-image depth attachment.
		MTL::Texture* m_DepthTexture = nullptr;
		uint32_t m_DepthWidth = 0;
		uint32_t m_DepthHeight = 0;
	};
} // namespace cbk::platform::metal
