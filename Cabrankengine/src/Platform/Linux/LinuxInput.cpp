#include <Platform/Linux/LinuxInput.h>
#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <GLFW/glfw3.h>

namespace cbk {

	Input* Input::s_Instance = new LinuxInput(); // TODO: who destroys this???

	bool LinuxInput::isKeyPressedImpl(KeyCode keyCode) {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		int state = glfwGetKey(window, keyCode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool LinuxInput::isMouseButtonPressedImpl(MouseCode mouseCode) {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		int state = glfwGetMouseButton(window, mouseCode);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> LinuxInput::getMousePositionImpl() {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return { static_cast<float>(xPos), static_cast<float>(yPos) }; // TODO: change every c-style cast to static_cast
	}

	float LinuxInput::getMouseXImpl() {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return static_cast<float>(xPos);
	}

	float LinuxInput::getMouseYImpl() {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return static_cast<float>(yPos);
	}

	void LinuxInput::setInputModeImpl(bool captureMouse, bool hideMouse) {
		auto window = static_cast<GLFWwindow*>(Application::get().getWindow().getNativeWindow());
		int flag = GLFW_CURSOR_NORMAL;
		if (captureMouse)
			flag = GLFW_CURSOR_DISABLED;
		else if (hideMouse)
			flag = GLFW_CURSOR_HIDDEN;
		glfwSetInputMode(window, GLFW_CURSOR, flag);
	}
} // namespace cbk