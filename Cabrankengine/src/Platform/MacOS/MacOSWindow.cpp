#include <pch.h>
#include "MacOSWindow.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <Cabrankengine/Events/MouseEvent.h>
#include <Cabrankengine/Events/ApplicationEvent.h>
#include <Cabrankengine/Events/KeyEvent.h>

namespace cbk {
	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description) {
		CBK_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Scope<Window> Window::create(const WindowProps& props) {
		return createScope<MacOSWindow>(props);
	}

	MacOSWindow::MacOSWindow(const WindowProps& props) {
		init(props);
	}

	MacOSWindow::~MacOSWindow() {
		shutdown();
	}

	void MacOSWindow::onUpdate() {
		glfwPollEvents();
	}

	void MacOSWindow::setVSync(bool enabled) {
		// if (enabled)
		//     glfwSwapInterval(1);
		// else
		//     glfwSwapInterval(0);
		m_Data.VSync = enabled;
	}

	bool MacOSWindow::isVSync() const {
		return m_Data.VSync;
	}

	rendering::GraphicsContext* MacOSWindow::getContext() const {
		return m_Context.get();
	}

	void MacOSWindow::init(const WindowProps& props) {
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		CBK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized) {
			int success = glfwInit();
			CBK_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			s_GLFWInitialized = true;
		}

		m_Window = glfwCreateWindow(props.Width, props.Height, m_Data.Title.c_str(), nullptr, nullptr);

		m_Context = rendering::GraphicsContext::create();
		m_Context->init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		setVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowResizeEvent event(width, height);
			data.EventCallback(event);
			data.Width = width;
			data.Height = height;
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action) {
				case GLFW_PRESS: {
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE: {
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT: {
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keyCode) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			KeyTypedEvent event(keyCode);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window,
		                           [](GLFWwindow* window, int button, int action, int mods) { // TODO: find out the use of action and mods.
			                           WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			                           switch (action) {
				                           case GLFW_PRESS: {
					                           MouseButtonPressedEvent event(button);
					                           data.EventCallback(event);
					                           break;
				                           }
				                           case GLFW_RELEASE: {
					                           MouseButtonReleasedEvent event(button);
					                           data.EventCallback(event);
					                           break;
				                           }
			                           }
		                           });

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
			data.EventCallback(event);
		});
	}

	void MacOSWindow::shutdown() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

} // namespace cbk
