#pragma once

#include <pch.h>

namespace cbk {

	class Event; // Forward declaration of the Event class

	// Basic properties of every Window implementation
	struct WindowProps {
		std::string Title;
		int Width;
		int Height;

		WindowProps(const std::string& title = "Cabrankengine", int width = 1600, int height = 900)
		    : Title(title), Width(width), Height(height) {}
	};

	// Window is an abstract class that defines the interface for a window in the Cabrankengine framework.
	class Window {
	  public:
		using EventCallbackFn = std::function<void(Event&)>; // Callback for the events. Here the signature is stated: it must get the event
		                                                     // as argument and return nothing

		virtual ~Window() = default;

		// Callback for the window update
		virtual void onUpdate() = 0;

		// Returns the window width
		[[nodiscard]] virtual int getWidth() const = 0;

		// Returns the window height
		[[nodiscard]] virtual int getHeight() const = 0;

		// Sets the callback function for the event polling
		virtual void setEventCallback(const EventCallbackFn& callback) = 0;

		// Sets whether VSync is enabled or not
		virtual void setVSync(bool enabled) = 0;

		// Returns whether VSync is enabled or not
		[[nodiscard]] virtual bool isVSync() const = 0;

		// Returns the platform specific window implementation.
		[[nodiscard]] virtual void* getNativeWindow() const = 0;

		// Only access to window creation (singleton pattern?)
		[[nodiscard]] static Scope<Window> create(const WindowProps& props = WindowProps());
	};
} // namespace cbk
