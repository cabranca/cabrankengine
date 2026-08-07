#pragma once

#include <Cabrankengine/Core/Input.h>

namespace cbk {

	class MacOSInput : public Input {
	  protected:
		// Returns whether a certain key is pressed.
		virtual bool isKeyPressedImpl(KeyCode keyCode) override;

		// Returns whether a certain mouse button is pressed.
		virtual bool isMouseButtonPressedImpl(MouseCode mouseCode) override;

		// Returns the current mouse position as a pair of floats (x, y).
		virtual std::pair<float, float> getMousePositionImpl() override;

		// Returns the current mouse X position.
		virtual float getMouseXImpl() override;

		// Returns the current mouse Y position.
		virtual float getMouseYImpl() override;

		void setInputModeImpl(bool captureMouse, bool hideMouse) override;
	};
} // namespace cbk