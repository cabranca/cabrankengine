#pragma once

#include <Cabrankengine/Core/Window.h>

namespace cbk::rendering {

	// GraphicsContext is an abstract class that defines the interface for a graphics context.
	class GraphicsContext {
	  public:
		virtual ~GraphicsContext() = default;

		// Initializes the graphics context.
		virtual void init(const Window& window) = 0;

		// Releases backend resources (device, allocator, instance, etc.). Must be
		// called before the windowing system tears down the surface.
		virtual void shutdown() = 0;

		[[nodiscard]] static Scope<GraphicsContext> create();
	};
} // namespace cbk::rendering
