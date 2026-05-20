#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

#include "VulkanSwapchainManager.h"

namespace cbk::platform::vk {

	class VulkanRendererAPI : public rendering::RendererAPI {
	  public:
		void init() override;
		void shutdown() override;
		void setClearColor(const math::Vector4& color) override;
		void clear() override;
		void beginFrame() override;
		void draw(const Ref<rendering::GeometryDescriptor>& dec) override;
		void drawIndexed(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& desc,
		                 const math::Mat4& transform, uint32_t indexCount = 0) override;
		void endFrame() override;
		void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		[[nodiscard]] static API getAPI();

		[[nodiscard]] uint32_t getMinImageCount() const;
		[[nodiscard]] uint32_t getImageCount() const;
		[[nodiscard]] VkCommandBuffer getCommandBuffer() const;

	  private:
		static constexpr uint32_t k_MaxFramesInFlight{ 2 };
		VulkanSwapchainManager m_SwapchainManager;
		uint32_t m_FrameIndex{ 0 };
		uint32_t m_ImageIndex{ 0 };
		std::array<VkFence, k_MaxFramesInFlight> m_Fences;
		std::array<VkSemaphore, k_MaxFramesInFlight> m_ImageAcquiredSemaphores;
		std::vector<VkSemaphore> m_RenderCompleteSemaphores;
		VkCommandPool m_CommandPool{ VK_NULL_HANDLE };
		std::array<VkCommandBuffer, k_MaxFramesInFlight> m_CommandBuffers;
		bool m_UpdateSwapchain{ false };
		math::Vector4 m_ClearColor{ 0.5, 0.5, 0.5, 1.f };

		bool createSyncObjects(VulkanDeviceContext* ctx);
		bool createRenderCompleteSemaphores();
		bool createCommandPool(VulkanDeviceContext* ctx);

		bool syncAndAcquire();
		bool commitRenderCommands(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& vertexArray,
		                          const math::Mat4& transform);
		bool submitQueue();

		void updateSwapchain(VulkanDeviceContext* ctx);
	};

} // namespace cbk::platform::vk
