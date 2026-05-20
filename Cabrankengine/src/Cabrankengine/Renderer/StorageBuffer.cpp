#include <pch.h>
#include "StorageBuffer.h"

#ifdef CBK_RENDERER_METAL
	#include <Platform/Metal/MetalStorageBuffer.h>
#endif

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLStorageBuffer.h>
#endif

namespace cbk::rendering {

	Ref<StorageBuffer> StorageBuffer::create(uint32_t size) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLStorageBuffer>(size);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalStorageBuffer>(size);
#elif defined(CBK_RENDERER_VULKAN)
        return nullptr; // TODO: Vulkan StorageBuffer not yet implemented
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering
