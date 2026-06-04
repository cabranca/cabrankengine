#pragma once

#include <Cabrankengine/Renderer/GraphicsContext.h>

// Forward declarations
namespace MTL {
	class Device;
	class CommandQueue;
} // namespace MTL

namespace cbk::platform::metal {

	class MetalDeviceContext : public rendering::GraphicsContext {
	  public:
		~MetalDeviceContext();

		// Initializes the graphics context.
		void init() override;
		void shutdown() override;

		[[nodiscard]] MTL::Device* getDevice() const;
		[[nodiscard]] MTL::CommandQueue* getCommandQueue() const;

	  private:
		MTL::Device* m_Device;             // I think I'm not the owner so I don't have to free
		MTL::CommandQueue* m_CommandQueue; // I think I'm not the owner so I don't have to free
	};
} // namespace cbk::platform::metal
