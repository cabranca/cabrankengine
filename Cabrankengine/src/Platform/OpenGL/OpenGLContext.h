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
		// Initializes the graphics context.
		void init() override;

		// OpenGL has no explicit context teardown beyond glfwDestroyWindow, so this
		// is a no-op. Implemented to satisfy the GraphicsContext interface.
		void shutdown() override;

	  private:
		GLFWwindow* m_WindowHandle; // Handle to the GLFW window associated with this context
	};
} // namespace cbk::platform::opengl
