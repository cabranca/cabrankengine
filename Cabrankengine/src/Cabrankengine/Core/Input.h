#pragma once

#include <pch.h>

#include "Core.h"
#include "KeyCodes.h"
#include "MouseCodes.h"

namespace cbk {

	// This class is used to perform input polling.
	// On one hand, the class is abstract so it can be implemented for different platforms (e.g., Windows, Linux, etc.) such as the renderer
	// related classes. On the other hand, it is a singleton so that we can access it from anywhere in the code. As we need to access it
	// from anywhere, the methods are static. That's why there is an indirection through a static instance pointer.
	class Input {
	  public:
		virtual ~Input() = default;

		// Returns whether a certain key is pressed.
		[[nodiscard]] static bool isKeyPressed(KeyCode keyCode) {
			return s_Instance->isKeyPressedImpl(keyCode);
		}

		// Returns whether a certain mouse button is pressed.
		[[nodiscard]] static bool isMouseButtonPressed(MouseCode mouseCode) {
			return s_Instance->isMouseButtonPressedImpl(mouseCode);
		}

		// Returns the current mouse position as a pair of floats (x, y).
		[[nodiscard]] static std::pair<float, float> getMousePosition() {
			return s_Instance->getMousePositionImpl();
		}

		// Returns the current mouse X position.
		[[nodiscard]] static float getMouseX() {
			return s_Instance->getMouseXImpl();
		}

		// Returns the current mouse Y position.
		[[nodiscard]] static float getMouseY() {
			return s_Instance->getMouseYImpl();
		}

		static void setInputMode(bool captureMouse, bool hideMouse) {
			return s_Instance->setInputModeImpl(captureMouse, hideMouse);
		}

	  protected:
		[[nodiscard]] virtual bool isKeyPressedImpl(KeyCode keyCode) = 0;
		[[nodiscard]] virtual bool isMouseButtonPressedImpl(MouseCode mouseCode) = 0;
		[[nodiscard]] virtual std::pair<float, float> getMousePositionImpl() = 0;
		[[nodiscard]] virtual float getMouseXImpl() = 0;
		[[nodiscard]] virtual float getMouseYImpl() = 0;
		virtual void setInputModeImpl(bool captureMouse, bool hideMouse) = 0;

	  private:
		// This could be initialized inline but it is initialized in the platform specific implementation file.
		// This is not the approach for the renderer but it could be.
		static Input* s_Instance;
	};
} // namespace cbk