#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

#include "VulkanConstants.h"
#include "VulkanSwapchainManager.h"

namespace cbk::platform::vk {

	class VulkanRendererAPI : public rendering::RendererAPI {
	  public:
		void init() override;
		void shutdown() override;
		void setClearColor(const math::Vector4& color) override;
		void beginFrame() override;
		void draw(const Ref<rendering::GeometryDescriptor>& desc) override;
		void drawIndexed(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& desc,
		                 const math::Mat4& transform, uint32_t indexCount = 0) override;
		void endScenePass() override;
		void endFrame() override;
		void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		[[nodiscard]] static API getAPI();

		[[nodiscard]] uint32_t getMinImageCount() const;
		[[nodiscard]] uint32_t getImageCount() const;
		[[nodiscard]] VkCommandBuffer getCommandBuffer() const;
		[[nodiscard]] uint64_t getFinalFrame() const override;

	  private:
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

		// Offscreen colour target the scene is rendered into, so the result can be sampled
		// by ImGui instead of going straight to the swapchain. One per frame in flight:
		// the UI pass of frame N samples the same image the scene pass of frame N+1 would
		// otherwise be overwriting while N is still executing. Sized to the window, so it
		// is recreated alongside the swapchain.
		std::array<VkImage, k_MaxFramesInFlight> m_FinalFrameImages{};
		std::array<VmaAllocation, k_MaxFramesInFlight> m_FinalFrameAllocations{};
		std::array<VkImageView, k_MaxFramesInFlight> m_FinalFrameViews{};
		std::array<VkDescriptorSet, k_MaxFramesInFlight> m_FinalFrameDescriptorSets{};
		VkExtent2D m_FinalFrameExtent{};

		// False when beginFrame() bailed out (minimized, or the swapchain went out of date),
		// which leaves no command buffer recording for endScenePass()/endFrame() to add to.
		bool m_FrameStarted{ false };

		// Initialization
		void createSyncObjects(VulkanDeviceContext* ctx);
		void createRenderCompleteSemaphores();
		void createCommandPool(VulkanDeviceContext* ctx);
		void createFinalFrameTargets(VulkanDeviceContext* ctx);
		void destroyFinalFrameTargets(VulkanDeviceContext* ctx);
		void ensureFinalFrameDescriptorSets();

		// Begin Frame stage
		bool syncAndAcquire();
		void setBufferObjectsCurrentFrame();
		void resetAndBeginCmdBuffer();
		void setupInitialBarriers();
		void beginRecording();
		void setViewportAndScissor();

		// Commands Queueing stage
		void commitRenderCommands(const Ref<rendering::Material>& material, const Ref<rendering::GeometryDescriptor>& desc,
		                          const math::Mat4& transform);

		void recordMaterial(const Ref<rendering::Material>& material, const math::Mat4& transform);
		void bindAndDraw(const Ref<rendering::GeometryDescriptor>& desc);

		// End Frame stage
		void submitQueue();
	};

} // namespace cbk::platform::vk
