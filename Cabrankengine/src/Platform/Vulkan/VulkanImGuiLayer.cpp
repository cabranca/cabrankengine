#include <pch.h>
#include <Cabrankengine/ImGui/ImGuiLayer.h>

#include <volk/volk.h> // must precede any other Vulkan header to establish VK_NO_PROTOTYPES
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/RenderCommand.h>

#include "VulkanDeviceContext.h"
#include "VulkanRendererAPI.h"

namespace cbk {

	ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

	void ImGuiLayer::onAttach() {
		CBK_PROFILE_FUNCTION();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		static_cast<void>(io); // TODO: wtf is this?
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.f;
			style.Colors[ImGuiCol_WindowBg].w = 1.f;
		}

		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		auto ctx = static_cast<platform::vk::VulkanDeviceContext*>(Application::get().getWindow().getContext());

		ImGui_ImplGlfw_InitForVulkan(window, true);
		auto rendererAPI = static_cast<platform::vk::VulkanRendererAPI*>(rendering::RenderCommand::getRendererAPI());

		// ImGui shallow-copies InitInfo; pColorAttachmentFormats must outlive the
		// Init call, so the format lives in a static.
		static VkFormat imguiColorFormat = ctx->getImageFormat();
		// The frame's vkCmdBeginRendering binds a depth attachment, so ImGui's pipeline
		// must declare the matching depthAttachmentFormat even though it doesn't test depth.
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			                                                  .colorAttachmentCount = 1,
			                                                  .pColorAttachmentFormats = &imguiColorFormat,
			                                                  .depthAttachmentFormat = ctx->getDepthFormat() };
		ImGui_ImplVulkan_InitInfo initInfo{ .ApiVersion = VK_API_VERSION_1_3,
			                                .Instance = ctx->getInstance(),
			                                .PhysicalDevice = ctx->getPhysicalDevice(),
			                                .Device = ctx->getLogicalDevice(),
			                                .QueueFamily = ctx->getQueueFamily(),
			                                .Queue = ctx->getDeviceQueue(),
			                                .MinImageCount = rendererAPI->getMinImageCount(),
			                                .ImageCount = rendererAPI->getImageCount(),
			                                .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			                                .DescriptorPoolSize = 1000,
			                                .UseDynamicRendering = true,
			                                .PipelineRenderingCreateInfo = pipelineRenderingCI };
		ImGui_ImplVulkan_Init(&initInfo);
	}

	void ImGuiLayer::onDetach() {
		CBK_PROFILE_FUNCTION();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::begin() {
		CBK_PROFILE_FUNCTION();

		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplVulkan_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::end() {
		CBK_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::get();
		io.DisplaySize = ImVec2(static_cast<float>(app.getWindow().getWidth()), static_cast<float>(app.getWindow().getHeight()));

		ImGui::Render();
		auto rendererAPI = static_cast<platform::vk::VulkanRendererAPI*>(rendering::RenderCommand::getRendererAPI());
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), rendererAPI->getCommandBuffer());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
} // namespace cbk
