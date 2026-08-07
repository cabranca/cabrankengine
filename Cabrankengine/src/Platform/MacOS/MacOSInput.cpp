#include "MacOSInput.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <GLFW/glfw3.h>

namespace cbk {

	Input* Input::s_Instance = new MacOSInput(); // TODO: who destroys this???

	bool MacOSInput::isKeyPressedImpl(KeyCode keyCode) {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		int state = glfwGetKey(window, keyCode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool MacOSInput::isMouseButtonPressedImpl(MouseCode mouseCode) {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		int state = glfwGetMouseButton(window, mouseCode);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> MacOSInput::getMousePositionImpl() {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return { static_cast<float>(xPos), static_cast<float>(yPos) }; // TODO: change every c-style cast to static_cast
	}

	float MacOSInput::getMouseXImpl() {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return static_cast<float>(xPos);
	}

	float MacOSInput::getMouseYImpl() {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return static_cast<float>(yPos);
	}

	void MacOSInput::setInputModeImpl(bool captureMouse, bool hideMouse) {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		int flag = GLFW_CURSOR_NORMAL;
		if (captureMouse)
			flag = GLFW_CURSOR_DISABLED;
		else if (hideMouse)
			flag = GLFW_CURSOR_HIDDEN;
		glfwSetInputMode(window, GLFW_CURSOR, flag);
	}
} // namespace cbk