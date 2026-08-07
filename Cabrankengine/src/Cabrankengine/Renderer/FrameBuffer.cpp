#include <pch.h>
#include "FrameBuffer.h"

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalFrameBuffer.h>
#endif

#ifdef CBK_RENDERER_OPENGL
#include <Platform/OpenGL/OpenGLFrameBuffer.h>
#endif

namespace cbk::rendering {

	Ref<FrameBuffer> FrameBuffer::create(const FrameBufferSpecification& spec) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLFrameBuffer>(spec);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalFrameBuffer>(spec);
#elif defined(CBK_RENDERER_VULKAN)
		return nullptr; // TODO: Vulkan FrameBuffer not yet implemented
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering
