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

	namespace {
		// Font size in ImGui is expressed in pixels, so a fixed value tracks physical
		// display density rather than resolution. This is the size intended for a
		// 1.0-scale (~96 DPI) display; the monitor's content scale multiplies it.
		constexpr float k_KBaseFontSize = 16.f;
	} // namespace

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

		// Keeps fonts and window sizes following whichever monitor a viewport lands on.
		io.ConfigDpiScaleFonts = true;
		io.ConfigDpiScaleViewports = true;

		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());

		// Reports 1.0 on macOS (handled via FramebufferScale) and on X11 sessions that
		// leave Xft.dpi unset, in which case scaling falls back to style.FontScaleMain.
		const float dpiScale = ImGui_ImplGlfw_GetContentScaleForWindow(window);
		CBK_CORE_INFO("ImGui display content scale: {0}", dpiScale);

		// ImGui 1.92 rasterizes glyphs on demand; the backend uploads the atlas
		// during RenderDrawData, so no explicit Build()/font-texture call is needed.
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/ocraext.ttf", k_KBaseFontSize * dpiScale);

		ImGui::StyleColorsDark();

		// Fonts alone leave padding and scrollbars at 1.0-scale sizes, which still reads
		// as a cramped UI on a high-DPI display.
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(dpiScale);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.f;
			style.Colors[ImGuiCol_WindowBg].w = 1.f;
		}

		auto ctx = static_cast<platform::vk::VulkanDeviceContext*>(Application::get().getWindow().getContext());

		ImGui_ImplGlfw_InitForVulkan(window, true);
		auto rendererAPI = static_cast<platform::vk::VulkanRendererAPI*>(rendering::RenderCommand::getRendererAPI());

		// ImGui shallow-copies InitInfo; pColorAttachmentFormats must outlive the
		// Init call, so the format lives in a static.
		static VkFormat s_ImguiColorFormat = ctx->getImageFormat();
		// The frame's vkCmdBeginRendering binds a depth attachment, so ImGui's pipeline
		// must declare the matching depthAttachmentFormat even though it doesn't test depth.
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			                                                  .colorAttachmentCount = 1,
			                                                  .pColorAttachmentFormats = &s_ImguiColorFormat,
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
			GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backupCurrentContext);
		}
	}
} // namespace cbk
