#pragma once

#include <Cabrankengine/Renderer/RendererAPI.h>

#include "VulkanConstants.h"
#include "VulkanDeviceContext.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanSwapchainManager.h"

namespace cbk::platform::vk {

	class VulkanRendererAPI : public rendering::RendererAPI {
	  public:
		void init(const Window& window) override;
		void shutdown() override;
		void setClearColor(const math::Vector4& color) override;
		void beginFrame() override;
		void beginScene(const math::Mat4& viewProjectionMatrix, const math::Vector3& cameraWorldPosition, const math::Vector3& direction,
		                const math::Vector3& radiance) override;
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

		// Accessor for Vulkan resource classes (textures, buffers, shaders, materials) that
		// need the device/allocator but sit below RendererAPI in the dependency graph and
		// have no other route to it now that Window no longer owns the graphics context.
		[[nodiscard]] static VulkanDeviceContext& getContext();
		[[nodiscard]] static VkDescriptorSet getPhongDescriptorSet();

	  private:
		VulkanDeviceContext m_Context;
		inline static VulkanDeviceContext* s_Context = nullptr; // Why not a static reference directly?
		VulkanSwapchainManager m_SwapchainManager;
		// Same split as m_Context/s_Context: the member owns the lifetime (created in init(),
		// destroyed in shutdown(), never at static-init time), the pointer is only a published
		// access path for the static accessors below and is null outside init()/shutdown().
		VulkanGraphicsPipeline m_GraphicsPipeline;
		inline static VulkanGraphicsPipeline* s_GraphicsPipeline = nullptr;
		uint32_t m_FrameIndex{ 0 };
		uint32_t m_ImageIndex{ 0 };
		std::array<VkFence, k_MaxFramesInFlight> m_Fences;
		std::array<VkSemaphore, k_MaxFramesInFlight> m_ImageAcquiredSemaphores;
		std::array<VkCommandBuffer, k_MaxFramesInFlight> m_CommandBuffers;
		math::Vector4 m_ClearColor{ 0.5, 0.5, 0.5, 1.f };

		// ImGui texture handles for the swapchain manager's per-frame-in-flight offscreen
		// color attachment (see VulkanSwapchainManager::getColorImageView), so the scene the
		// engine renders can be displayed inside an ImGui window instead of going straight to
		// the swapchain. Registered lazily in ensureFinalFrameDescriptorSets() and invalidated
		// whenever the swapchain (and therefore the underlying image views) is rebuilt.
		std::array<VkDescriptorSet, k_MaxFramesInFlight> m_FinalFrameDescriptorSets{};

		// False when beginFrame() bailed out (minimized, or the swapchain went out of date),
		// which leaves no command buffer recording for endScenePass()/endFrame() to add to.
		bool m_FrameStarted{ false };

		// Initialization
		void createSyncObjects();
		void createRenderCompleteSemaphores();
		void createCommandPool();
		void destroyFinalFrameDescriptorSets();
		void ensureFinalFrameDescriptorSets();

		// Begin Frame stage
		bool syncAndAcquire();
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
