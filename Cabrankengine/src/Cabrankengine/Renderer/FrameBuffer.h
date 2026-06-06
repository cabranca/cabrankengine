#pragma once

namespace cbk::rendering {

	struct FrameBufferSpecification {
		uint32_t width, height;
		uint32_t samples = 1;
		bool swapChainTarget = false;
	};

	class FrameBuffer {
	  public:
		virtual ~FrameBuffer() = default;

		virtual void bind() = 0;
		virtual void unbind() = 0;

		[[nodiscard]] virtual const FrameBufferSpecification& getSpecification() const = 0;
		[[nodiscard]] virtual uint32_t getColorAttachmentRendererID() const = 0;

		[[nodiscard]] static Ref<FrameBuffer> create(const FrameBufferSpecification& spec);
	};
} // namespace cbk::rendering
