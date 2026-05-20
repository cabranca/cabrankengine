#pragma once

#include <Cabrankengine/Scene/Scene.h>
#include <Cabrankengine/Renderer/RenderLayer.h>

#include "LayerStack.h"

namespace cbk {

	// Forward declarations
	class Event; // Base class for all events
	class ImGuiLayer; // Overlay for ImGui rendering
	class Window; // Base Window class
	class WindowCloseEvent; // Event triggered when the close window button is pressed.
	class WindowResizeEvent; // Event triggered when the window is resized
	

	class Application {
		public:
			Application();
			virtual ~Application(); // Cannot define the destructor in the header because of the incomplete types (Window)

			// Main Application running loop
			void Run();

			// Callback for the event system
			void OnEvent(Event& e);

			// Pushes a layer to the stack on top of the other layers (always under the overlays)
			void pushLayer(Layer* layer);

			// Pushes a layer to the stack on top of the other overlays
			void pushOverlay(Layer* layer);

			// Pops the top layer from the stack
			void popLayer(Layer* layer);

			// Pops the top overlay from the stack
			void popOverlay(Layer* layer);

			// Returns a reference to the Window
			Window& getWindow() { return *m_Window; }

			// Returns a ref to the game scene
			scene::Scene& getScene() { return m_Scene; }
			const scene::Scene& getScene() const { return m_Scene; }

			// Loads a scene with its register and resets systems
			void loadScene(scene::Scene scene) { m_Scene = std::move(scene); m_RenderLayer->setScene(&m_Scene); }

			// Returns a Ref to the ECS Registry
			ecs::Registry* getRegistry() { return m_Scene.getRegistry(); }
	
			// Returns a reference to the app (Singleton-ish pattern)
			static Application& get() { return *s_Instance; }

		private:
			// Callback for the WindowClose Event
			bool onWindowClose(WindowCloseEvent& e);

			// Callback for the WindowResize Event
			bool onWindowResize(WindowResizeEvent& e);

			std::unique_ptr<Window> m_Window; // Ptr to the app window
			ImGuiLayer* m_ImGuiLayer = nullptr; // ImGui layer for rendering the UI (null on Vulkan for now)
			rendering::RenderLayer* m_RenderLayer;
			bool m_Running; // Whether the app must stop or not
			LayerStack m_LayerStack; // Stack of layers to forward the events to
			float m_LastFrameTime; // Time of the last frame
			bool m_Minimized = false; // Whether the window is minimized or not
			scene::Scene m_Scene; // Scene with the game world and registry
			
			inline static Application* s_Instance = nullptr; // Static instance of the app (Singleton-ish pattern)
	};

	// To be defined in client. Only way to create the app. It's similar to singleton but it allows more instances.
	// This is a function and not a method because it decouples the EntryPoint from the Application class.
	Application* createApplication();
}
