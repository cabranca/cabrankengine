#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

namespace cbk::platform::vk {

	class VulkanRendererAPI : public rendering::RendererAPI {
	  public:
		void init() override;
		void setClearColor(const math::Vector4& color) override;
		void clear() override;
		void draw(const Ref<rendering::VertexArray>& vertexArray) override;
		void drawIndexed(const Ref<rendering::VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		void endFrame() override;
		void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		static API getAPI() {
			return API::Vulkan;
		}

	  private:
	  	static constexpr uint32_t k_MaxFramesInFlight{ 2 };
		uint32_t m_FrameIndex{ 0 };
		uint32_t m_ImageIndex{ 0 };
		bool m_UpdateSwapchain{ false };

		bool createVulkanInstance();
		bool createVulkanDevice();
		bool createAllocator();
		bool createSwapchain();
		bool getSwapchainImages();
		bool createDepthAttachment();
		bool createDepthImage();
		bool createSyncObjects();
		bool createRenderCompleteSemaphores();
		bool createCommandPool();

		bool syncAndAcquire();
		bool commitRenderCommands(const Ref<rendering::VertexArray>& vertexArray);
		bool submitQueue();

		void updateSwapchain();
	};

} // namespace cbk::platform::vk
