#include <pch.h>
#include "StorageBuffer.h"

#ifdef CBK_RENDERER_METAL
	#include <Platform/Metal/MetalStorageBuffer.h>
#endif

#if defined(CBK_RENDERER_OPENGL) && !defined(CBK_OPENGL_ES)
	#include <Platform/OpenGL/OpenGLStorageBuffer.h>
#endif

namespace cbk::rendering {

	Ref<StorageBuffer> StorageBuffer::create(uint32_t size) {
#if defined(CBK_RENDERER_OPENGL) && !defined(CBK_OPENGL_ES)
		return createRef<platform::opengl::OpenGLStorageBuffer>(size);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalStorageBuffer>(size);
#else
		// SSBOs are not available on WebGL 2 / GLES 3.0 — caller must handle null.
		return nullptr;
#endif
	}
} // namespace cbk::rendering
