#include <pch.h>

#include "VulkanRendererAPI.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/Materials/Material.h>

#include "IVulkanRecordable.h"
#include "VkCheck.h"
#include "VulkanDeviceContext.h"
#include "VulkanGeometryDescriptor.h"
#include "VulkanPBRMaterial.h"
#include "VulkanPhongMaterial.h"
#include "VulkanStorageBuffer.h"
#include "VulkanTexture2DMaterial.h"
#include "VulkanTextMaterial.h"
#include "VulkanUniformBuffer.h"

#include <Cabrankengine/Renderer/Renderer.h>
#include <Cabrankengine/Renderer/StorageBuffer.h>
#include <Cabrankengine/Renderer/UniformBuffer.h>

namespace cbk::platform::vk {

	using namespace math;
	using namespace rendering;

	void VulkanRendererAPI::init() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		m_SwapchainManager.init(ctx);
		createSyncObjects(ctx);
		createCommandPool(ctx);
	}

	void VulkanRendererAPI::shutdown() {
		// Tear down
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		vkDeviceWaitIdle(ctx->getLogicalDevice());

		// Destroy per-material-class pipeline state before the device is torn down.
		VulkanPBRMaterial::destroySharedResources();
		VulkanPhongMaterial::destroySharedResources();
		VulkanTexture2DMaterial::destroySharedResources();
		VulkanTextMaterial::destroySharedResources();

		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			vkDestroyFence(ctx->getLogicalDevice(), m_Fences[i], nullptr);
			vkDestroySemaphore(ctx->getLogicalDevice(), m_ImageAcquiredSemaphores[i], nullptr);
		}
		for (auto i = 0; i < m_RenderCompleteSemaphores.size(); i++)
			vkDestroySemaphore(ctx->getLogicalDevice(), m_RenderCompleteSemaphores[i], nullptr);

		m_SwapchainManager.shutdown();
		vkDestroyCommandPool(ctx->getLogicalDevice(), m_CommandPool, nullptr);
	}

	void VulkanRendererAPI::setClearColor(const Vector4& color) {
		m_ClearColor = color;
	}

	// No-op on Vulkan. With dynamic rendering the clear is encoded into
	// VK_ATTACHMENT_LOAD_OP_CLEAR inside beginFrame()'s VkRenderingAttachmentInfo,
	// using the color stored by setClearColor(). Nothing to do here at draw time.
	void VulkanRendererAPI::clear() {}

	void VulkanRendererAPI::beginFrame() {
		if (!syncAndAcquire())
			return;

		// Point the scene UBO at this frame's slot before any setData call from
		// Renderer::beginScene — prevents the CPU from trampling a UBO the GPU is
		// still reading from for the prior in-flight frame. Same rationale for the
		// point-light SSBO, which Renderer::beginScene rewrites every frame.
		auto sceneUbo = static_cast<VulkanUniformBuffer*>(rendering::Renderer::getSceneUBO().get());
		sceneUbo->setCurrentFrame(m_FrameIndex);
		auto lightSSBO = static_cast<VulkanStorageBuffer*>(rendering::Renderer::getLightSSBO().get());
		lightSSBO->setCurrentFrame(m_FrameIndex);

		auto cb = m_CommandBuffers[m_FrameIndex];
		auto vkResult = vkResetCommandBuffer(cb, 0);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::beginFrame(): error resetting command buffer ({})", static_cast<int>(vkResult));
			return;
		}
		VkCommandBufferBeginInfo cbBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		vkResult = vkBeginCommandBuffer(cb, &cbBI);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::beginFrame(): error beginning command buffer ({})", static_cast<int>(vkResult));
			return;
		}
		std::array<VkImageMemoryBarrier2, 2> outputBarriers{
			VkImageMemoryBarrier2{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                       .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                       .srcAccessMask = 0,
			                       .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                       .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                       .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                       .image = m_SwapchainManager.getSwapchainImage(m_ImageIndex),
			                       .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } },
			VkImageMemoryBarrier2{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                       .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			                       .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			                       .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			                       .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			                       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                       .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                       .image = m_SwapchainManager.getDepthImage(m_ImageIndex),
			                       .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			                                          .levelCount = 1,
			                                          .layerCount = 1 } }
		};
		VkDependencyInfo barrierDependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                                    .imageMemoryBarrierCount = 2,
			                                    .pImageMemoryBarriers = outputBarriers.data() };
		vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);

		// Clearing happens here via VK_ATTACHMENT_LOAD_OP_CLEAR — m_ClearColor is set by setClearColor().
		VkRenderingAttachmentInfo colorAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = m_SwapchainManager.getSwapchainImageView(m_ImageIndex),
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			                                           .clearValue{
			                                               .color{ m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w } } };
		VkRenderingAttachmentInfo depthAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = m_SwapchainManager.getDepthImageView(m_ImageIndex),
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			                                           .clearValue = { .depthStencil = { 1.0f, 0 } } };

		auto&            window = Application::get().getWindow();
		VkRenderingInfo  renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			                            .renderArea{ .extent{ .width = static_cast<uint32_t>(window.getWidth()),
			                                                  .height = static_cast<uint32_t>(window.getHeight()) } },
			                            .layerCount = 1,
			                            .colorAttachmentCount = 1,
			                            .pColorAttachments = &colorAttachmentInfo,
			                            .pDepthAttachment = &depthAttachmentInfo };
		vkCmdBeginRendering(cb, &renderingInfo);
		VkViewport vp{ .width = static_cast<float>(window.getWidth()),
			           .height = static_cast<float>(window.getHeight()),
			           .minDepth = 0.0f,
			           .maxDepth = 1.0f };
		vkCmdSetViewport(cb, 0, 1, &vp);
		VkRect2D scissor{ .extent{ .width = static_cast<uint32_t>(window.getWidth()),
			                       .height = static_cast<uint32_t>(window.getHeight()) } };
		vkCmdSetScissor(cb, 0, 1, &scissor);
	}

	void VulkanRendererAPI::draw(const Ref<GeometryDescriptor>& vertexArray) {}

	void VulkanRendererAPI::drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform,
	                                    uint32_t indexCount) {
		// Records the per-draw work into the active command buffer set up by beginFrame.
		// No CB lifecycle, no submit, no present — those belong to beginFrame/endFrame.
		commitRenderCommands(material, desc, transform);
	}

	void VulkanRendererAPI::endFrame() {
		auto cb = m_CommandBuffers[m_FrameIndex];
		vkCmdEndRendering(cb);
		VkImageMemoryBarrier2 barrierPresent{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                  .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                  .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                                  .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                  .dstAccessMask = 0,
			                                  .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                  .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			                                  .image = m_SwapchainManager.getSwapchainImage(m_ImageIndex),
			                                  .subresourceRange{
			                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		VkDependencyInfo barrierPresentDependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                                           .imageMemoryBarrierCount = 1,
			                                           .pImageMemoryBarriers = &barrierPresent };
		vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
		auto vkResult = vkEndCommandBuffer(cb);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::endFrame(): error ending command buffer ({})", static_cast<int>(vkResult));
			return;
		}

		if (!submitQueue())
			return;

		if (m_UpdateSwapchain) {
			m_UpdateSwapchain = false;
			m_SwapchainManager.updateSwapchain();
			auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
			for (auto& semaphore: m_RenderCompleteSemaphores)
				vkDestroySemaphore(ctx->getLogicalDevice(), semaphore, nullptr);
			createRenderCompleteSemaphores();
		}
	}

	void VulkanRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

	void VulkanRendererAPI::createSyncObjects(VulkanDeviceContext* ctx) {
		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fenceCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			VK_CHECK(vkCreateFence(ctx->getLogicalDevice(), &fenceCI, nullptr, &m_Fences[i]));
			VK_CHECK(vkCreateSemaphore(ctx->getLogicalDevice(), &semaphoreCI, nullptr, &m_ImageAcquiredSemaphores[i]));
		}
		createRenderCompleteSemaphores();
	}

	void VulkanRendererAPI::createRenderCompleteSemaphores() {
		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		m_RenderCompleteSemaphores.resize(m_SwapchainManager.getSwapchainImagesSize());
		for (auto& semaphore: m_RenderCompleteSemaphores)
			VK_CHECK(vkCreateSemaphore(ctx->getLogicalDevice(), &semaphoreCI, nullptr, &semaphore));
	}

	void VulkanRendererAPI::createCommandPool(VulkanDeviceContext* ctx) {
		VkCommandPoolCreateInfo commandPoolCI{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			                                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			                                   .queueFamilyIndex = ctx->getQueueFamily() };
		VK_CHECK(vkCreateCommandPool(ctx->getLogicalDevice(), &commandPoolCI, nullptr, &m_CommandPool));
		VkCommandBufferAllocateInfo cbAllocCI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			                                   .commandPool = m_CommandPool,
			                                   .commandBufferCount = k_MaxFramesInFlight };
		VK_CHECK(vkAllocateCommandBuffers(ctx->getLogicalDevice(), &cbAllocCI, m_CommandBuffers.data()));
	}

	bool VulkanRendererAPI::syncAndAcquire() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		auto vkResult = vkWaitForFences(ctx->getLogicalDevice(), 1, &m_Fences[m_FrameIndex], true, UINT64_MAX);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::syncAndAcquire(): error waiting for fences ({})", static_cast<int>(vkResult));
			return false;
		}
		vkResult = vkResetFences(ctx->getLogicalDevice(), 1, &m_Fences[m_FrameIndex]);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::syncAndAcquire(): error reseting fences ({})", static_cast<int>(vkResult));
			return false;
		}

		vkResult = vkAcquireNextImageKHR(ctx->getLogicalDevice(), *m_SwapchainManager.getSwapchain(), UINT64_MAX,
		                                 m_ImageAcquiredSemaphores[m_FrameIndex], VK_NULL_HANDLE, &m_ImageIndex);
		if (vkResult < VK_SUCCESS) {
			if (vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
				m_UpdateSwapchain = true;
				return false;
			}
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error waiting for fences ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::commitRenderCommands(const Ref<Material>& material, const Ref<rendering::GeometryDescriptor>& desc,
	                                             const math::Mat4& transform) {
		// Per-draw recording only. The command buffer was reset, begun, transitioned,
		// and vkCmdBeginRendering'd by beginFrame(); vkCmdEndRendering + endCB + submit + present
		// happen in endFrame(). Multiple draws within the same frame share that lifecycle.
		auto cb = m_CommandBuffers[m_FrameIndex];

		auto recordable = dynamic_cast<IVulkanRecordable*>(material.get());
		CBK_CORE_ASSERT(recordable, "VulkanRendererAPI: material does not implement IVulkanRecordable");

		VkPipeline       pipeline = recordable->getPipeline();
		VkPipelineLayout layout   = recordable->getPipelineLayout();

		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Set 0: scene globals (view-projection, dir light, camera pos).
		auto sceneUbo = static_cast<VulkanUniformBuffer*>(rendering::Renderer::getSceneUBO().get());
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, sceneUbo->getDescriptorSet(), 0, nullptr);

		// Set 1: per-material descriptor (textures).
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, recordable->getDescriptorSet(), 0, nullptr);

		// Set 2: point-light SSBO. Only bound for pipelines whose layout actually
		// declares it (currently PBR); other materials would fail descriptor-set-
		// compatibility validation if we bound a set their layout doesn't expect.
		if (recordable->wantsLightSSBO()) {
			auto lightSSBO = static_cast<VulkanStorageBuffer*>(rendering::Renderer::getLightSSBO().get());
			vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, lightSSBO->getDescriptorSet(), 0, nullptr);
		}

		// Per-draw model matrix at offset 0. stageFlags must cover every stage of the
		// overlapping range in the pipeline layout (VS|FS, see VulkanPhong/PBRMaterial).
		vkCmdPushConstants(cb, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(math::Mat4), &transform);

		// Material-specific fragment-stage push constants + lazy descriptor refresh.
		recordable->recordCommandBuffer(cb, layout);

		auto vkDesc = static_cast<VulkanGeometryDescriptor*>(desc.get());
		vkDesc->bindBuffers(cb);
		// (indexCount, instanceCount=1, firstIndex=0, vertexOffset=0, firstInstance=0).
		// instanceCount must be ≥ 1 — passing 0 silently produces zero draws.
		vkCmdDrawIndexed(cb, vkDesc->getIndexCount(), 1, 0, 0, 0);

		return true;
	}
	bool VulkanRendererAPI::submitQueue() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_ImageAcquiredSemaphores[m_FrameIndex],
			.pWaitDstStageMask = &waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_CommandBuffers[m_FrameIndex],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_RenderCompleteSemaphores[m_ImageIndex],
		};
		auto vkResult = vkQueueSubmit(ctx->getDeviceQueue(), 1, &submitInfo, m_Fences[m_FrameIndex]);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error submitting queue ({})", static_cast<int>(vkResult));
			return false;
		}
		m_FrameIndex = (m_FrameIndex + 1) % k_MaxFramesInFlight;
		VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			                          .waitSemaphoreCount = 1,
			                          .pWaitSemaphores = &m_RenderCompleteSemaphores[m_ImageIndex],
			                          .swapchainCount = 1,
			                          .pSwapchains = m_SwapchainManager.getSwapchain(),
			                          .pImageIndices = &m_ImageIndex };
		vkResult = vkQueuePresentKHR(ctx->getDeviceQueue(), &presentInfo);
		// SUBOPTIMAL still presented the frame; OUT_OF_DATE did not. Both mean the
		// swapchain no longer matches the surface — flag it for recreation and let
		// endFrame() run the update block (returning false would skip it).
		if (vkResult == VK_SUBOPTIMAL_KHR || vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
			m_UpdateSwapchain = true;
			return true;
		}
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error presenting queue ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	RendererAPI::API VulkanRendererAPI::getAPI() {
		return API::Vulkan;
	}

	uint32_t VulkanRendererAPI::getMinImageCount() const {
		return m_SwapchainManager.getMinImageCount();
	}
	
	uint32_t VulkanRendererAPI::getImageCount() const {
		return m_SwapchainManager.getSwapchainImagesSize();
	}

	VkCommandBuffer VulkanRendererAPI::getCommandBuffer() const {
		return m_CommandBuffers.at(m_FrameIndex);
	}
} // namespace cbk::platform::vk
