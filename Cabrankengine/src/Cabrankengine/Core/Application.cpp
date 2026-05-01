#include <pch.h>
#include "Application.h"

#include <GLFW/glfw3.h>

#include <Cabrankengine/Config/Config.h>
#include <Cabrankengine/Events/ApplicationEvent.h>
#include <Cabrankengine/ImGui/ImGuiLayer.h>
#include <Cabrankengine/Renderer/Renderer.h>
#include <Cabrankengine/Renderer/RenderCommand.h>
#include <Cabrankengine/Scene/DefaultLibrary.h>

#include "AudioEngine.h"
#include "Logger.h"
#include "Timestep.h"
#include "Window.h"

namespace cbk {

	using namespace ecs;
	using namespace scene;
	using namespace rendering;

	Application::Application() : m_Running(true), m_LastFrameTime(0.0f)
	{
		CBK_PROFILE_FUNCTION();
		CBK_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		nlohmann::json configJson = Config::load("config.json");

		WindowProps props(configJson.at("window").at("title"), configJson.at("window").at("width"), configJson.at("window").at("height"));

		m_Window = std::unique_ptr<Window>(Window::create(props));
		m_Window->setEventCallback(BIND_EVENT_FN(&Application::OnEvent, this));

		//AudioEngine::init();

		Renderer::init();
		DefaultLibrary::init();

		m_RenderLayer = new RenderLayer();
		RenderLayer::setScene(&m_Scene);
		pushLayer(m_RenderLayer);

		m_ImGuiLayer = new ImGuiLayer();
		pushOverlay(m_ImGuiLayer);
	}

	Application::~Application() {
		CBK_PROFILE_FUNCTION();
		Renderer::shutdown();
	}

	void Application::Run()
	{
		CBK_PROFILE_FUNCTION();

		while (m_Running) {
			CBK_PROFILE_SCOPE("RunLoop");

			float time = static_cast<float>(glfwGetTime()); // This should be in Platform::getTime() or similar
			Timestep timestep = time - m_LastFrameTime; // Calculate the time since the last frame
			m_LastFrameTime = time;

			if (!m_Minimized) {
				CBK_PROFILE_SCOPE("LayerStack OnUpdate");

				for (Layer* layer : m_LayerStack)
					layer->onUpdate(timestep);
			}
			 
			m_ImGuiLayer->begin();
			{
				CBK_PROFILE_SCOPE("LayerStack OnImGuiRender");
				for (Layer* layer : m_LayerStack)
					layer->onImGuiRender();
			}
			m_ImGuiLayer->end();

			// Finalize the frame's GPU commands (commit command buffer, present drawable).
			// This is a no-op on OpenGL. On Metal it must happen exactly once per frame,
			// after all rendering (Renderer, Renderer2D, TextRenderer, ImGui) is complete.
			RenderCommand::endFrame();

			m_Window->onUpdate();
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(&Application::onWindowClose, this));
		dispatcher.dispatch<WindowResizeEvent>(BIND_EVENT_FN(&Application::onWindowResize, this));
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) {
			(*--it)->onEvent(e);
			if (e.handled())
				break;
		}
	}

	void Application::pushLayer(Layer* layer) {
		CBK_PROFILE_FUNCTION();
		m_LayerStack.pushLayer(layer);
		layer->onAttach();
	}

	void Application::pushOverlay(Layer* layer) {
		CBK_PROFILE_FUNCTION();
		m_LayerStack.pushOverlay(layer);
		layer->onAttach();
	}

	void Application::popLayer(Layer* layer) {
		m_LayerStack.popLayer(layer);
	}

	void Application::popOverlay(Layer* layer) {
		m_LayerStack.popOverlay(layer);
	}

	bool Application::onWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::onWindowResize(WindowResizeEvent& e)
	{
		CBK_PROFILE_FUNCTION();
		if (e.getWidth() == 0 || e.getHeight() == 0) {
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;
		Renderer::onWindowResize(e.getWidth(), e.getHeight());
		return true;
	}
}


