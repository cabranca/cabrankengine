#pragma once

#include <pch.h>

struct GLFWwindow;

namespace cbk::rendering {

	// GraphicsContext is an abstract class that defines the interface for a graphics context.
	class GraphicsContext {
		public:
			virtual ~GraphicsContext() = default;

			// Initializes the graphics context.
			virtual void init() = 0;

			// Releases backend resources (device, allocator, instance, etc.). Must be
			// called before the windowing system tears down the surface.
			virtual void shutdown() = 0;

			// Buffer swapping is the process of presenting the rendered image to the screen
			// while also preparing the next frame for rendering.
			virtual void swapBuffers() = 0;

			static Scope<GraphicsContext> create(GLFWwindow* windowHandle);
	};
}
