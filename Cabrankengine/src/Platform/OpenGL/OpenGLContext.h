#pragma once

#include <Cabrankengine/Renderer/GraphicsContext.h>

// Forward declarations
struct GLFWwindow;

namespace cbk::platform::opengl {

	class OpenGLContext : public rendering::GraphicsContext {
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		// Initializes the graphics context.
		virtual void init() override;

		// OpenGL has no explicit context teardown beyond glfwDestroyWindow, so this
		// is a no-op. Implemented to satisfy the GraphicsContext interface.
		virtual void shutdown() override {}

		// Buffer swapping is the process of presenting the rendered image to the screen
		// while also preparing the next frame for rendering.
		virtual void swapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle; // Handle to the GLFW window associated with this context
	};
}
