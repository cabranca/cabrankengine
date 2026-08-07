#include <pch.h>

#include "VulkanRendererAPI.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

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
#include <vulkan/vulkan_core.h>

namespace cbk::platform::vk {

	using namespace math;
	using namespace rendering;

	void VulkanRendererAPI::init() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		m_SwapchainManager.init(ctx);
		createSyncObjects(ctx);
		createCommandPool(ctx);
		createFinalFrameTargets(ctx);
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

		destroyFinalFrameTargets(ctx);
		m_SwapchainManager.shutdown();
		vkDestroyCommandPool(ctx->getLogicalDevice(), m_CommandPool, nullptr);
	}

	void VulkanRendererAPI::setClearColor(const Vector4& color) {
		m_ClearColor = color;
	}

	void VulkanRendererAPI::beginFrame() {
		// Cannot happen in init(): ImGui's descriptor pool only exists once ImGuiLayer has
		// been attached, which is after RenderCommand::init() runs. See the method comment.
		ensureFinalFrameDescriptorSets();

		if (!syncAndAcquire())
			return;

		// Sets the UBO and SSBO current frame for when Renderer::beginFrame() is called.
		setBufferObjectsCurrentFrame();

		resetAndBeginCmdBuffer();
		setupInitialBarriers();
		beginRecording();
		setViewportAndScissor();

		m_FrameStarted = true;
	}

	void VulkanRendererAPI::draw(const Ref<GeometryDescriptor>& vertexArray) {}

	void VulkanRendererAPI::drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform,
	                                    uint32_t indexCount) {
		// Records the per-draw work into the active command buffer set up by beginFrame.
		// No CB lifecycle, no submit, no present — those belong to beginFrame/endFrame.
		commitRenderCommands(material, desc, transform);
	}

	void VulkanRendererAPI::endScenePass() {
		if (!m_FrameStarted)
			return;

		auto cb = m_CommandBuffers[m_FrameIndex];

		// 1. Close the offscreen pass the scene was drawn into.
		vkCmdEndRendering(cb);

		// 2. Hand the result over to the fragment shader: ImGui samples this image in the
		// UI pass below, so the colour writes have to complete and the layout has to move
		// to the one the descriptor was registered with.
		VkImageMemoryBarrier2 barrierSample{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                 .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                 .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                                 .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			                                 .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
			                                 .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                 .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			                                 .image = m_FinalFrameImages[m_FrameIndex],
			                                 .subresourceRange{
			                                     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		VkDependencyInfo barrierSampleDependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                                          .imageMemoryBarrierCount = 1,
			                                          .pImageMemoryBarriers = &barrierSample };
		vkCmdPipelineBarrier2(cb, &barrierSampleDependencyInfo);

		// 3. Open the UI pass, this time against the swapchain image. No depth attachment —
		// ImGui does not depth-test, and its pipeline declares VK_FORMAT_UNDEFINED to match.
		VkRenderingAttachmentInfo colorAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = m_SwapchainManager.getSwapchainImageView(m_ImageIndex),
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			                                           .clearValue{
			                                               .color{ m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w } } };

		auto& window = Application::get().getWindow();
		VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			                           .renderArea{ .extent{ .width = static_cast<uint32_t>(window.getWidth()),
			                                                 .height = static_cast<uint32_t>(window.getHeight()) } },
			                           .layerCount = 1,
			                           .colorAttachmentCount = 1,
			                           .pColorAttachments = &colorAttachmentInfo,
			                           .pDepthAttachment = nullptr };
		vkCmdBeginRendering(cb, &renderingInfo);
	}

	void VulkanRendererAPI::endFrame() {
		// Nothing was recorded if beginFrame() bailed out, but the swapchain may still be
		// waiting to be rebuilt below — that is what got us here in the first place.
		if (m_FrameStarted) {
			auto cb = m_CommandBuffers[m_FrameIndex];

			// 1. End rendering
			vkCmdEndRendering(cb);

			// 2. Wait that the color attachment has been finished writing and
			// prepare it for presentation
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

			// 3. End the command buffer.
			VK_CHECK(vkEndCommandBuffer(cb));

			// 4. Submit queue.
			submitQueue();

			m_FrameStarted = false;
		}

		// 5. Recreate the swapchain if it's in an invalid state.
		if (m_UpdateSwapchain) {
			m_UpdateSwapchain = false;
			m_SwapchainManager.updateSwapchain();
			auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
			for (auto& semaphore: m_RenderCompleteSemaphores)
				vkDestroySemaphore(ctx->getLogicalDevice(), semaphore, nullptr);
			createRenderCompleteSemaphores();

			// The offscreen targets are window-sized, so they follow the swapchain.
			// updateSwapchain() has already waited for the device to go idle.
			destroyFinalFrameTargets(ctx);
			createFinalFrameTargets(ctx);
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

	void VulkanRendererAPI::createFinalFrameTargets(VulkanDeviceContext* ctx) {
		auto& window = Application::get().getWindow();
		const auto width = static_cast<uint32_t>(window.getWidth());
		const auto height = static_cast<uint32_t>(window.getHeight());

		// A minimized window reports 0, which is not a legal image extent.
		m_FinalFrameExtent = { .width = width > 0 ? width : 1, .height = height > 0 ? height : 1 };

		// The format has to be the one every material pipeline declared in its
		// VkPipelineRenderingCreateInfo, and they all take it from the swapchain.
		// COLOR_ATTACHMENT to render into it, SAMPLED so ImGui can read it back.
		VkImageCreateInfo imageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = ctx->getImageFormat(),
			.extent{ .width = m_FinalFrameExtent.width, .height = m_FinalFrameExtent.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };

		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			VK_CHECK(vmaCreateImage(ctx->getAllocator(), &imageCI, &allocCI, &m_FinalFrameImages[i], &m_FinalFrameAllocations[i], nullptr));
			VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                          .image = m_FinalFrameImages[i],
				                          .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                          .format = ctx->getImageFormat(),
				                          .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
			VK_CHECK(vkCreateImageView(ctx->getLogicalDevice(), &viewCI, nullptr, &m_FinalFrameViews[i]));
		}
	}

	void VulkanRendererAPI::destroyFinalFrameTargets(VulkanDeviceContext* ctx) {
		// The descriptor sets belong to ImGui's pool, which may already be gone by the time
		// the renderer is torn down — the context outliving us is not guaranteed either way.
		const bool imGuiAlive = ImGui::GetCurrentContext() != nullptr;

		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			if (imGuiAlive && m_FinalFrameDescriptorSets[i] != VK_NULL_HANDLE)
				ImGui_ImplVulkan_RemoveTexture(m_FinalFrameDescriptorSets[i]);
			m_FinalFrameDescriptorSets[i] = VK_NULL_HANDLE;

			if (m_FinalFrameViews[i] != VK_NULL_HANDLE)
				vkDestroyImageView(ctx->getLogicalDevice(), m_FinalFrameViews[i], nullptr);
			m_FinalFrameViews[i] = VK_NULL_HANDLE;

			if (m_FinalFrameImages[i] != VK_NULL_HANDLE)
				vmaDestroyImage(ctx->getAllocator(), m_FinalFrameImages[i], m_FinalFrameAllocations[i]);
			m_FinalFrameImages[i] = VK_NULL_HANDLE;
			m_FinalFrameAllocations[i] = VK_NULL_HANDLE;
		}
	}

	void VulkanRendererAPI::ensureFinalFrameDescriptorSets() {
		// ImGui_ImplVulkan_AddTexture allocates out of a descriptor pool that only exists
		// after ImGui_ImplVulkan_Init, and ImGuiLayer::onAttach runs *after* the renderer's
		// init() — calling this any earlier dereferences a null backend. Registering on the
		// first frame instead also covers re-registering after a swapchain resize, and the
		// null check keeps it from allocating a fresh set every frame.
		if (ImGui::GetCurrentContext() == nullptr)
			return;

		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			if (m_FinalFrameDescriptorSets[i] == VK_NULL_HANDLE)
				m_FinalFrameDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(m_FinalFrameViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	bool VulkanRendererAPI::syncAndAcquire() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		VK_CHECK(vkWaitForFences(ctx->getLogicalDevice(), 1, &m_Fences[m_FrameIndex], true, UINT64_MAX));

		auto vkResult = vkAcquireNextImageKHR(ctx->getLogicalDevice(), *m_SwapchainManager.getSwapchain(), UINT64_MAX,
		                                      m_ImageAcquiredSemaphores[m_FrameIndex], VK_NULL_HANDLE, &m_ImageIndex);
		if (vkResult < VK_SUCCESS) {
			if (vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
				m_UpdateSwapchain = true;
				return false;
			}
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error waiting for fences ({})", static_cast<int>(vkResult));
			return false;
		}

		// Only reset once the acquire succeeded. Returning early with the fence unsignaled
		// leaves no submit to signal it, and the next frame's wait on this same slot would
		// then block forever.
		VK_CHECK(vkResetFences(ctx->getLogicalDevice(), 1, &m_Fences[m_FrameIndex]));
		return true;
	}

	void VulkanRendererAPI::setBufferObjectsCurrentFrame() {
		auto sceneUbo = static_cast<VulkanUniformBuffer*>(rendering::Renderer::getSceneUBO().get());
		sceneUbo->setCurrentFrame(m_FrameIndex);
		auto lightSSBO = static_cast<VulkanStorageBuffer*>(rendering::Renderer::getLightSSBO().get());
		lightSSBO->setCurrentFrame(m_FrameIndex);
	}

	void VulkanRendererAPI::resetAndBeginCmdBuffer() {
		auto cb = m_CommandBuffers[m_FrameIndex];

		VK_CHECK(vkResetCommandBuffer(cb, 0));

		VkCommandBufferBeginInfo cbBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		VK_CHECK(vkBeginCommandBuffer(cb, &cbBI));
	}

	void VulkanRendererAPI::setupInitialBarriers() {
		auto cb = m_CommandBuffers[m_FrameIndex];
		std::array<VkImageMemoryBarrier2, 3> outputBarriers{
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
			                                          .layerCount = 1 } },
			// The offscreen target ends every frame in SHADER_READ_ONLY_OPTIMAL, so it has to
			// come back to ATTACHMENT_OPTIMAL before the scene pass can write it again.
			// oldLayout UNDEFINED discards the old contents, which the CLEAR loadOp does
			// anyway, and saves having to track the true layout across the first frame.
			VkImageMemoryBarrier2{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                       .srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			                       .srcAccessMask = 0,
			                       .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                       .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                       .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                       .image = m_FinalFrameImages[m_FrameIndex],
			                       .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } }
		};
		VkDependencyInfo barrierDependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                                    .imageMemoryBarrierCount = static_cast<uint32_t>(outputBarriers.size()),
			                                    .pImageMemoryBarriers = outputBarriers.data() };
		vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
	}

	void VulkanRendererAPI::beginRecording() {
		auto cb = m_CommandBuffers.at(m_FrameIndex);
		VkRenderingAttachmentInfo colorAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = m_FinalFrameViews[m_FrameIndex],
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

		// The render area follows the offscreen target, not the window: they are the same
		// size today, but the scene pass no longer has anything to do with the swapchain.
		VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			                           .renderArea{ .extent = m_FinalFrameExtent },
			                           .layerCount = 1,
			                           .colorAttachmentCount = 1,
			                           .pColorAttachments = &colorAttachmentInfo,
			                           .pDepthAttachment = &depthAttachmentInfo };
		vkCmdBeginRendering(cb, &renderingInfo);
	}

	void VulkanRendererAPI::setViewportAndScissor() {
		auto cb = m_CommandBuffers.at(m_FrameIndex);
		VkViewport vp{ .width = static_cast<float>(m_FinalFrameExtent.width),
			           .height = static_cast<float>(m_FinalFrameExtent.height),
			           .minDepth = 0.0f,
			           .maxDepth = 1.0f };
		vkCmdSetViewport(cb, 0, 1, &vp);
		VkRect2D scissor{ .extent = m_FinalFrameExtent };
		vkCmdSetScissor(cb, 0, 1, &scissor);
	}

	void VulkanRendererAPI::commitRenderCommands(const Ref<Material>& material, const Ref<rendering::GeometryDescriptor>& desc,
	                                             const math::Mat4& transform) {
		recordMaterial(material, transform);
		bindAndDraw(desc);
	}

	void VulkanRendererAPI::recordMaterial(const Ref<rendering::Material>& material, const math::Mat4& transform) {
		auto cb = m_CommandBuffers[m_FrameIndex];

		// Each concrete material binds its own pipeline, descriptor sets and push
		// constants. The renderer only supplies the command buffer and the per-draw
		// model matrix; the set-index convention lives in VulkanDescriptorBinding.h.
		auto recordable = dynamic_cast<IVulkanRecordable*>(material.get());
		CBK_CORE_ASSERT(recordable, "VulkanRendererAPI: material does not implement IVulkanRecordable");

		recordable->record(cb, transform);
	}

	void VulkanRendererAPI::bindAndDraw(const Ref<rendering::GeometryDescriptor>& desc) {
		auto cb = m_CommandBuffers[m_FrameIndex];

		auto vkDesc = static_cast<VulkanGeometryDescriptor*>(desc.get());
		vkDesc->bindBuffers(cb);

		vkCmdDrawIndexed(cb, vkDesc->getIndexCount(), 1, 0, 0, 0);
	}

	void VulkanRendererAPI::submitQueue() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());

		// 1. Submit the command queue. The wait semaphore make the GPU wait until the presentation engine
		// is done with the previous image. The waitDstStageMask value implies that this waiting is needed
		// by the color attachment stage. Also a signal semaphore is given to signal when the rendering is done.
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
		VK_CHECK(vkQueueSubmit(ctx->getDeviceQueue(), 1, &submitInfo, m_Fences[m_FrameIndex]));

		// 2. Update frame index and present. It waits until the render complete semaphore is signaled.
		m_FrameIndex = (m_FrameIndex + 1) % k_MaxFramesInFlight;
		VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			                          .waitSemaphoreCount = 1,
			                          .pWaitSemaphores = &m_RenderCompleteSemaphores[m_ImageIndex],
			                          .swapchainCount = 1,
			                          .pSwapchains = m_SwapchainManager.getSwapchain(),
			                          .pImageIndices = &m_ImageIndex };
		VkResult vkResult = vkQueuePresentKHR(ctx->getDeviceQueue(), &presentInfo);
		// SUBOPTIMAL still presented the frame; OUT_OF_DATE did not. Both mean the
		// swapchain no longer matches the surface — flag it for recreation.
		if (vkResult == VK_SUBOPTIMAL_KHR || vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
			m_UpdateSwapchain = true;
			return;
		}
		if (vkResult != VK_SUCCESS)
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error presenting queue ({})", static_cast<int>(vkResult));
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

	uint64_t VulkanRendererAPI::getFinalFrame() const {
		// m_FrameIndex only advances in submitQueue(), so this is still the target the
		// scene pass wrote earlier in this same frame. Null until the first beginFrame().
		return reinterpret_cast<uint64_t>(m_FinalFrameDescriptorSets[m_FrameIndex]);
	}
} // namespace cbk::platform::vk
