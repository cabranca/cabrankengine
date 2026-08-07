#pragma once

#include <Cabrankengine/Renderer/FrameBuffer.h>

namespace cbk::platform::opengl {

	class OpenGLFrameBuffer : public rendering::FrameBuffer {
	  public:
		OpenGLFrameBuffer(const rendering::FrameBufferSpecification& spec);
		~OpenGLFrameBuffer();

		void bind() override;
		void unbind() override;

		virtual const rendering::FrameBufferSpecification& getSpecification() const override;
		virtual uint32_t getColorAttachmentRendererID() const override;

		// The current state is not valid so it must be recreated
		void invalidate();

	  private:
		uint32_t m_RendererID;
		uint32_t m_ColorAttachment, m_DepthAttachment;
		rendering::FrameBufferSpecification m_Specs;
	};
} // namespace cbk::platform::opengl