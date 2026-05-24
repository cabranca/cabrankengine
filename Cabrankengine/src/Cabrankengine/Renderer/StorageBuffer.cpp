#include <pch.h>
#include "StorageBuffer.h"

#ifdef CBK_RENDERER_METAL
	#include <Platform/Metal/MetalStorageBuffer.h>
#endif

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLStorageBuffer.h>
#endif

#ifdef CBK_RENDERER_VULKAN
	#include <Platform/Vulkan/VulkanStorageBuffer.h>
#endif

namespace cbk::rendering {

	Ref<StorageBuffer> StorageBuffer::create(uint32_t size) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLStorageBuffer>(size);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalStorageBuffer>(size);
#elif defined(CBK_RENDERER_VULKAN)
		// Point lights live in set=2 binding=0 of the PBR pipeline (see PBR.slang).
		return createRef<platform::vk::VulkanStorageBuffer>(size, /*binding=*/0);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering
