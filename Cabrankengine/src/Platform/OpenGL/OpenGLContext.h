#pragma once

#include <Cabrankengine/Renderer/GraphicsContext.h>

// Forward declarations
struct GLFWwindow;

namespace cbk::platform::opengl {

	// Drains glGetError() and routes any pending errors to the logger, prefixed with `where`.
	// Use as a checkpoint after suspect calls. On desktop this is redundant with the debug
	// callback installed in OpenGLContext::init(); on WebGL 2 it is the only option.
	void checkGLError(const char* where);

	class OpenGLContext : public rendering::GraphicsContext {
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		// Initializes the graphics context.
		virtual void init() override;

		// Buffer swapping is the process of presenting the rendered image to the screen
		// while also preparing the next frame for rendering.
		virtual void swapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle; // Handle to the GLFW window associated with this context
	};
}
