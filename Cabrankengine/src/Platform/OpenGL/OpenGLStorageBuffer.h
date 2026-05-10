#pragma once

#ifndef CBK_OPENGL_ES

#include <Cabrankengine/Renderer/StorageBuffer.h>

namespace cbk::platform::opengl {

	class OpenGLStorageBuffer : public rendering::StorageBuffer {
	  public:
		OpenGLStorageBuffer(uint32_t size);
		~OpenGLStorageBuffer() override;
		void setData(const void* data, uint32_t size) override;
		void bind(uint32_t bindingPoint) const override;

	  private:
		uint32_t m_RendererID;
	};
} // namespace cbk::platform::opengl

#endif // !CBK_OPENGL_ES