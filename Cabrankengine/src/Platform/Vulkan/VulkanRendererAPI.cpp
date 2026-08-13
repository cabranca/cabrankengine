#include <pch.h>

#include "VulkanRendererAPI.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/Materials/Material.h>

#include "IVulkanRecordable.h"
#include "VkCheck.h"
#include "VulkanCommands.h"
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

	void VulkanRendererAPI::init(const Window& window) {
		m_Context.init(window);
		s_Context = &m_Context;
		m_SwapchainManager.init(m_Context);
		createSyncObjects();
		auto commandBuffers = m_Context.getQueue().allocateCommandBuffers(k_MaxFramesInFlight);
		std::copy(commandBuffers.begin(), commandBuffers.end(), m_CommandBuffers.begin());
	}

	void VulkanRendererAPI::shutdown() {
		// Tear down
		const VkDevice device = m_Context.getDevice();
		m_Context.waitIdle();

		// Destroy per-material-class pipeline state before the device is torn down.
		VulkanPBRMaterial::destroySharedResources();
		VulkanPhongMaterial::destroySharedResources();
		VulkanTexture2DMaterial::destroySharedResources();
		VulkanTextMaterial::destroySharedResources();

		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			vkDestroyFence(device, m_Fences[i], nullptr);
			vkDestroySemaphore(device, m_ImageAcquiredSemaphores[i], nullptr);
		}

		destroyFinalFrameDescriptorSets();
		m_SwapchainManager.shutdown();
		m_Context.shutdown();
		s_Context = nullptr;
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
		vkCmdEndRendering(cb);

		ImageBarrierInfo info{ .Image = m_SwapchainManager.getResolveImage(m_FrameIndex),
			                   .OldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                   .NewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			                   .SrcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			                   .DstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
			                   .SrcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                   .DstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			                   .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			                   .MipLevels = 1 };

		VulkanCommands::transitionImageLayout(cb, { info });

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

			vkCmdEndRendering(cb);

			ImageBarrierInfo info{ .Image = m_SwapchainManager.getSwapchainImage(m_ImageIndex),
				                   .OldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				                   .NewLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				                   .SrcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				                   .DstAccessMask = 0,
				                   .SrcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				                   .DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				                   .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
				                   .MipLevels = 1 };

			VulkanCommands::transitionImageLayout(cb, { info });

			// 3. End the command buffer.
			VK_CHECK(vkEndCommandBuffer(cb));

			std::vector<VkPipelineStageFlags> waitDstStageMask = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

			m_Context.getQueue().submitCommands(cb, m_Fences[m_FrameIndex], m_ImageAcquiredSemaphores[m_FrameIndex],
			                                    m_SwapchainManager.getSemaphore(m_ImageIndex), waitDstStageMask);
			auto result = m_Context.getQueue().present(m_SwapchainManager.getSemaphore(m_ImageIndex), m_SwapchainManager.getSwapchain(),
			                                           m_ImageIndex);

			m_FrameStarted = false;
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
				m_SwapchainManager.recreateSwapchain();
				// The per-frame color image views just got destroyed and recreated, so the ImGui
				// texture handles registered against the old views are now dangling. Drop them;
				// ensureFinalFrameDescriptorSets() re-registers fresh ones on the next beginFrame().
				destroyFinalFrameDescriptorSets();
			}

			m_FrameIndex = (m_FrameIndex + 1) % k_MaxFramesInFlight;
			
		}
	}

	void VulkanRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

	void VulkanRendererAPI::createSyncObjects() {
		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fenceCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			VK_CHECK(vkCreateFence(m_Context.getDevice(), &fenceCI, nullptr, &m_Fences[i]));
			VK_CHECK(vkCreateSemaphore(m_Context.getDevice(), &semaphoreCI, nullptr, &m_ImageAcquiredSemaphores[i]));
		}
	}

	void VulkanRendererAPI::destroyFinalFrameDescriptorSets() {
		// The descriptor sets belong to ImGui's pool, which may already be gone by the time
		// the renderer is torn down — the context outliving us is not guaranteed either way.
		const bool imGuiAlive = ImGui::GetCurrentContext() != nullptr;

		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			if (imGuiAlive && m_FinalFrameDescriptorSets[i] != VK_NULL_HANDLE)
				ImGui_ImplVulkan_RemoveTexture(m_FinalFrameDescriptorSets[i]);
			m_FinalFrameDescriptorSets[i] = VK_NULL_HANDLE;
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
				m_FinalFrameDescriptorSets[i] =
				    ImGui_ImplVulkan_AddTexture(m_SwapchainManager.getResolveImageView(i), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	bool VulkanRendererAPI::syncAndAcquire() {
		VK_CHECK(vkWaitForFences(m_Context.getDevice(), 1, &m_Fences[m_FrameIndex], VK_TRUE, UINT64_MAX));

		m_ImageIndex = m_SwapchainManager.acquireImage(m_ImageAcquiredSemaphores[m_FrameIndex]);

		// The swapchain was out of date and got rebuilt; the fence stays signalled and the semaphore
		// unsignalled, so simply retry on the next frame.
		if (m_ImageIndex == UINT32_MAX)
			return false;

		// Only reset once the acquire succeeded. Returning early with the fence unsignaled
		// leaves no submit to signal it, and the next frame's wait on this same slot would
		// then block forever.
		VK_CHECK(vkResetFences(m_Context.getDevice(), 1, &m_Fences[m_FrameIndex]));
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

		ImageBarrierInfo swapchainColorBarrier{ .Image = m_SwapchainManager.getSwapchainImage(m_ImageIndex),
			                                    .OldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                                    .NewLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                    .SrcAccessMask = 0,
			                                    .DstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                                    .SrcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                    .DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                    .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			                                    .MipLevels = 1 };

		// D32_SFLOAT has no stencil component; including STENCIL_BIT in the aspect mask for a
		// depth-only format is invalid per VUID-VkImageMemoryBarrier2-subresourceRange-09601.
		const VkFormat depthFormat = m_Context.getDepthFormat();
		const bool depthHasStencil = depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT;
		const VkImageAspectFlags depthAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | (depthHasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

		ImageBarrierInfo sceneDepthBarrier{ .Image = m_SwapchainManager.getDepthImage(m_FrameIndex),
			                                .OldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                                .NewLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                .SrcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			                                .DstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			                                // DEPTH_STENCIL_ATTACHMENT_WRITE_BIT is only valid with a
			                                // fragment-test (or ALL_GRAPHICS/ALL_COMMANDS) stage — never
			                                // COLOR_ATTACHMENT_OUTPUT, per VUID-...-srcAccessMask-03913.
			                                .SrcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			                                .DstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			                                .AspectFlags = depthAspectFlags,
			                                .MipLevels = 1 };

		ImageBarrierInfo sceneColorBarrier{ .Image = m_SwapchainManager.getColorImage(m_FrameIndex),
			                                .OldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                                .NewLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                .SrcAccessMask = 0,
			                                .DstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                                .SrcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			                                .DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			                                .MipLevels = 1 };

		ImageBarrierInfo sceneResolveBarrier{ .Image = m_SwapchainManager.getResolveImage(m_FrameIndex),
			                                  .OldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                                  .NewLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                  .SrcAccessMask = 0,
			                                  .DstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                                  .SrcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			                                  .DstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                  .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			                                  .MipLevels = 1 };

		VulkanCommands::transitionImageLayout(cb, { swapchainColorBarrier, sceneDepthBarrier, sceneColorBarrier, sceneResolveBarrier });
	}

	void VulkanRendererAPI::beginRecording() {
		auto cb = m_CommandBuffers.at(m_FrameIndex);
		VkRenderingAttachmentInfo colorAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = m_SwapchainManager.getColorImageView(m_FrameIndex),
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
													   .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
													   .resolveImageView = m_SwapchainManager.getResolveImageView(m_FrameIndex),
													   .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			                                           .clearValue{
			                                               .color{ m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w } } };
		VkRenderingAttachmentInfo depthAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = m_SwapchainManager.getDepthImageView(m_FrameIndex),
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			                                           .clearValue = { .depthStencil = { 1.0f, 0 } } };

		// The render area follows the offscreen target, not the window: they are the same
		// size today, but the scene pass no longer has anything to do with the swapchain.
		VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			                           .renderArea{ .extent = m_SwapchainManager.getExtent() },
			                           .layerCount = 1,
			                           .colorAttachmentCount = 1,
			                           .pColorAttachments = &colorAttachmentInfo,
			                           .pDepthAttachment = &depthAttachmentInfo };
		vkCmdBeginRendering(cb, &renderingInfo);
	}

	void VulkanRendererAPI::setViewportAndScissor() {
		auto cb = m_CommandBuffers.at(m_FrameIndex);
		const VkExtent2D extent = m_SwapchainManager.getExtent();
		VkViewport vp{
			.width = static_cast<float>(extent.width), .height = static_cast<float>(extent.height), .minDepth = 0.0f, .maxDepth = 1.0f
		};
		vkCmdSetViewport(cb, 0, 1, &vp);
		VkRect2D scissor{ .extent = extent };
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

	VulkanDeviceContext& VulkanRendererAPI::getContext() {
		CBK_CORE_ASSERT(s_Context, "VulkanRendererAPI::getContext() called before init()");
		return *s_Context;
	}

	uint64_t VulkanRendererAPI::getFinalFrame() const {
		// m_FrameIndex only advances in submitQueue(), so this is still the target the
		// scene pass wrote earlier in this same frame. Null until the first beginFrame().
		return reinterpret_cast<uint64_t>(m_FinalFrameDescriptorSets[m_FrameIndex]);
	}
} // namespace cbk::platform::vk
