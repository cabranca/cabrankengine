#pragma once

#include <Cabrankengine/Renderer/GraphicsContext.h>

// Forward declarations
namespace MTL {
	class Device;
	class CommandQueue;
} // namespace MTL
namespace CA {
	class MetalLayer;
	class MetalDrawable;
}
struct GLFWwindow;

namespace cbk::platform::metal {

	class MetalContext : public rendering::GraphicsContext {
	  public:
		MetalContext(GLFWwindow* windowHandle);
		~MetalContext();

		// Initializes the graphics context.
		void init() override;
		void shutdown() override;

		// Buffer swapping is the process of presenting the rendered image to the screen
		// while also preparing the next frame for rendering.
		void swapBuffers() override;

		MTL::Device* getDevice() const { return m_Device; }
        MTL::CommandQueue* getCommandQueue() const { return m_CommandQueue; }
        CA::MetalLayer* getSwapchain() const { return m_Swapchain; }
		CA::MetalDrawable* getCurrentDrawable();

	  private:
		GLFWwindow* m_WindowHandle; // Handle to the GLFW window associated with this context

		MTL::Device* m_Device;
		MTL::CommandQueue* m_CommandQueue;
		CA::MetalLayer* m_Swapchain;
		CA::MetalDrawable* m_CurrentDrawable = nullptr;
	};
} // namespace cbk::platform::metal
