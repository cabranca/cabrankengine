#include <pch.h>
#include <Cabrankengine/ImGui/ImGuiLayer.h>

#import <Metal/Metal.h>
#include <backends/imgui_impl_metal.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/RenderCommand.h>

#include "MetalDeviceContext.h"
#include "MetalRendererAPI.h"

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

		auto* window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		auto* ctx = static_cast<platform::metal::MetalDeviceContext*>(Application::get().getWindow().getContext());

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		auto objcDevice = (__bridge id<MTLDevice>)ctx->getDevice();
		ImGui_ImplMetal_Init(objcDevice);
	}

	void ImGuiLayer::onDetach() {
		CBK_PROFILE_FUNCTION();

		ImGui_ImplMetal_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::begin() {
		CBK_PROFILE_FUNCTION();

		auto* rendererAPI = static_cast<platform::metal::MetalRendererAPI*>(rendering::RenderCommand::getRendererAPI());

		// Unlike OpenGL/Vulkan, Metal's NewFrame requires the active RenderPassDescriptor for the current frame.
		// Bridge the metal-cpp handle to the Objective-C type ImGui's backend expects (toll-free, no ownership transfer).
		ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)rendererAPI->getRenderPassDescriptor());
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::end() {
		CBK_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();

		// DisplaySize and DisplayFramebufferScale are set by ImGui_ImplGlfw_NewFrame()
		// (HiDPI-aware); don't override them here or the UI scale/hit-testing breaks.
		ImGui::Render();

		auto* rendererAPI = static_cast<platform::metal::MetalRendererAPI*>(rendering::RenderCommand::getRendererAPI());

		// Metal rendering requires the command buffer and the active render command encoder.
		// Bridge the metal-cpp handles to the Objective-C types ImGui's backend expects.
		ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id<MTLCommandBuffer>)rendererAPI->getCommandBuffer(),
		                               (__bridge id<MTLRenderCommandEncoder>)rendererAPI->getCommandEncoder());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
} // namespace cbk