#include "LinuxWindow.h"
#include <Cabrankengine/Core/Logger.h>
#include <Cabrankengine/Events/MouseEvent.h>
#include <Cabrankengine/Events/ApplicationEvent.h>
#include <Cabrankengine/Events/KeyEvent.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Platform/OpenGL/OpenGLContext.h>

namespace cbk {
    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char* description) {
		CBK_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

    Scope<Window> Window::create(const WindowProps& props) {
        return createScope<LinuxWindow>(props);
    }

    LinuxWindow::LinuxWindow(const WindowProps& props) {
        init(props);
    }

    LinuxWindow::~LinuxWindow() {
        shutdown();
    }

    void LinuxWindow::onUpdate() {
        glfwPollEvents();
#ifdef CBK_RENDERER_OPENGL
        m_Context->swapBuffers();
#endif
        // Vulkan presents via vkQueuePresentKHR inside the RendererAPI; no GLFW swap call here.
    }

    void LinuxWindow::setVSync(bool enabled) {
#ifdef CBK_RENDERER_OPENGL
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
#endif
        // Vulkan: vsync is the swapchain present mode (FIFO_KHR), not a GLFW knob.
        m_Data.VSync = enabled;
    }

    bool LinuxWindow::isVSync() const {
        return m_Data.VSync;
    }

	rendering::GraphicsContext* LinuxWindow::getContext() const {
        return m_Context.get();
    }

	void LinuxWindow::init(const WindowProps& props) {
		m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        CBK_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

        if (!s_GLFWInitialized) {
            int success = glfwInit();
            CBK_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }

#ifdef CBK_RENDERER_VULKAN
        // Vulkan owns the surface, so GLFW must not create an OpenGL context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

        m_Window = glfwCreateWindow(props.Width, props.Height, m_Data.Title.c_str(), nullptr, nullptr);
        
        m_Context = rendering::GraphicsContext::create(m_Window);
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

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) { // TODO: find out the use of action and mods.
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

	void LinuxWindow::shutdown() {
        // Tear down the graphics backend (Vulkan device/allocator/instance, or no-op
        // for OpenGL) before the surface/window goes away.
        if (m_Context)
            m_Context->shutdown();
        glfwDestroyWindow(m_Window);
    }

}
