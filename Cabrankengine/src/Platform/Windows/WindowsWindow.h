#pragma once

#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/GraphicsContext.h>

struct GLFWwindow; // Forward declaration of the GLFWwindow struct

namespace cbk {

	class WindowsWindow : public Window {
	  public:
		WindowsWindow(const WindowProps& props);
		~WindowsWindow();

		// Callback for the window update
		void onUpdate() override;

		// Returns the window width
		int getWidth() const override {
			return m_Data.Width;
		}

		// Returns the window height
		int getHeight() const override {
			return m_Data.Height;
		}

		// Sets the callback function for the event polling
		void setEventCallback(const EventCallbackFn& callback) override {
			m_Data.EventCallback = callback;
		}

		// Sets whether VSync is enabled or not
		void setVSync(bool enabled) override;

		// Returns whether VSync is enabled or not
		bool isVSync() const override;

		// Returns the Windows specific window.
		void* getNativeWindow() const override {
			return m_Window;
		}

		rendering::GraphicsContext* getContext() const override;

	  private:
		// Initialize the window from the given properties
		void init(const WindowProps& props);

		// Shutdown GLFW correctly to destroy the window
		void shutdown();

		GLFWwindow* m_Window;                        // Actual window object
		Scope<rendering::GraphicsContext> m_Context; // Graphics context for rendering

		// Data of the window to work with GLFW
		// TODO: could I reuse WindowProps in any way?
		struct WindowData {
			std::string Title;
			int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data; // Current window data
	};
} // namespace cbk
