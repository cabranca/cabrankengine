#include <pch.h>
#include "WindowsWindow.h"

#include <GLFW/glfw3.h>

#include <Cabrankengine/Events/ApplicationEvent.h>
#include <Cabrankengine/Events/KeyEvent.h>
#include <Cabrankengine/Events/MouseEvent.h>

namespace cbk {

	using namespace rendering;

	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description) {
		CBK_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Scope<Window> Window::create(const WindowProps& props) {
		return createScope<WindowsWindow>(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props) {
		CBK_PROFILE_FUNCTION();

		init(props);
	}

	WindowsWindow::~WindowsWindow() {
		CBK_PROFILE_FUNCTION();

		shutdown();
	}

	void WindowsWindow::onUpdate() {
		glfwPollEvents();
	}

	void WindowsWindow::setVSync(bool enabled) {
		CBK_PROFILE_FUNCTION();

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);
		m_Data.VSync = enabled;
	}

	bool WindowsWindow::isVSync() const {
		return m_Data.VSync;
	}

	rendering::GraphicsContext* WindowsWindow::getContext() const {
		return m_Context.get();
	}

	void WindowsWindow::init(const WindowProps& props) {
		CBK_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		CBK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized) {
			CBK_PROFILE_FUNCTION();

			int success = glfwInit();
			CBK_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}

		{
			CBK_PROFILE_FUNCTION();

			m_Window = glfwCreateWindow(props.Width, props.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}

		m_Context = GraphicsContext::create();
		m_Context->init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		setVSync(true);

		// Set GLFW callbacks: https://www.glfw.org/docs/3.3/input_guide.html
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

	void WindowsWindow::shutdown() {
		CBK_PROFILE_FUNCTION();

		// Tear down the graphics backend (Vulkan device/allocator/instance, or no-op
		// for OpenGL) before the surface/window goes away.
		if (m_Context)
			m_Context->shutdown();
		glfwDestroyWindow(m_Window);
	}
} // namespace cbk
