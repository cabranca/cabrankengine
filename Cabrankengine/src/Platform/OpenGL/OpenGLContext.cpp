#include <pch.h>
#include "OpenGLContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace cbk::platform::opengl {

#ifndef __EMSCRIPTEN__
	static void GLAPIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/,
	                                       const GLchar* message, const void* /*userParam*/) {
		// Notification-level spam is mostly buffer-bind chatter; skip it.
		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
			return;
		const char* sev = severity == GL_DEBUG_SEVERITY_HIGH     ? "HIGH"
		                  : severity == GL_DEBUG_SEVERITY_MEDIUM ? "MED"
		                  : severity == GL_DEBUG_SEVERITY_LOW    ? "LOW"
		                                                         : "NOTE";
		CBK_CORE_ERROR("GL [{0}] src=0x{1:x} type=0x{2:x} id={3}: {4}", sev, source, type, id, message);
	}
#endif

	void OpenGLContext::init() {
		CBK_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
#ifndef __EMSCRIPTEN__
		// WebGL function pointers are resolved automatically by emscripten; calling glad would fail.
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		CBK_CORE_ASSERT(status, "Failed to initialize Glad!");
#endif

		CBK_CORE_INFO("OpenGL Info:");
		CBK_CORE_INFO("  Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		CBK_CORE_INFO("  Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		CBK_CORE_INFO("  Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
		CBK_CORE_INFO("  GLSL: {0}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

#ifndef __EMSCRIPTEN__
		// Asynchronous routing of every GL error through our logger. Desktop GL 4.3+ only;
		// WebGL 2 has no equivalent — use checkGLError() at suspect call sites instead.
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLDebugCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif
	}

	void OpenGLContext::shutdown() {}

	void checkGLError(const char* where) {
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			const char* name = err == GL_INVALID_ENUM                    ? "GL_INVALID_ENUM"
			                   : err == GL_INVALID_VALUE                 ? "GL_INVALID_VALUE"
			                   : err == GL_INVALID_OPERATION             ? "GL_INVALID_OPERATION"
			                   : err == GL_INVALID_FRAMEBUFFER_OPERATION ? "GL_INVALID_FRAMEBUFFER_OPERATION"
			                   : err == GL_OUT_OF_MEMORY                 ? "GL_OUT_OF_MEMORY"
			                                                             : "unknown";
			CBK_CORE_ERROR("GL error at {0}: {1} (0x{2:x})", where, name, err);
		}
	}
} // namespace cbk::platform::opengl
