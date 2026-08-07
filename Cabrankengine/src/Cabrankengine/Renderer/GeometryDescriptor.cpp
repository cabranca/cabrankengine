#include <pch.h>
#include "GeometryDescriptor.h"

#ifdef CBK_RENDERER_OPENGL
#include <Platform/OpenGL/OpenGLVertexArray.h>
#endif

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalGeometryDescriptor.h>
#endif

#ifdef CBK_RENDERER_VULKAN
#include <Platform/Vulkan/VulkanGeometryDescriptor.h>
#endif

namespace cbk::rendering {

	Ref<GeometryDescriptor> GeometryDescriptor::create(const void* vertexData, size_t vertexDataSize, const void* indexData,
	                                                   size_t indexDataSize, const VertexLayout& layout) {
#ifdef CBK_RENDERER_OPENGL
		return createScope<platform::opengl::OpenGLVertexArray>(vertexData, vertexDataSize, indexData, indexDataSize, layout);
#elif defined(CBK_RENDERER_METAL)
		return createScope<platform::metal::MetalGeometryDescriptor>(vertexData, vertexDataSize, indexData, indexDataSize, layout);
#elif defined(CBK_RENDERER_VULKAN)
		return createScope<platform::vk::VulkanGeometryDescriptor>(vertexData, vertexDataSize, indexData, indexDataSize, layout);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	Ref<GeometryDescriptor> GeometryDescriptor::create(size_t vertexDataSize, const void* indexData, size_t indexDataSize,
	                                                   const VertexLayout& layout) {
#ifdef CBK_RENDERER_OPENGL
		return createScope<platform::opengl::OpenGLVertexArray>(vertexDataSize, indexData, indexDataSize, layout);
#elif defined(CBK_RENDERER_METAL)
		return createScope<platform::metal::MetalGeometryDescriptor>(vertexDataSize, indexData, indexDataSize, layout);
#elif defined(CBK_RENDERER_VULKAN)
		return createScope<platform::vk::VulkanGeometryDescriptor>(vertexDataSize, indexData, indexDataSize, layout);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering