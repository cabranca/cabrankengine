#pragma once

#include <cstdint>

namespace cbk::platform::metal {

	// Metal argument-table buffer-index convention shared by every material PSO and
	// its matching .metal shader. Vertex and fragment argument tables are
	// independent in Metal, so the same index means different things per stage.
	// Centralising the constants here mirrors Vulkan's VulkanDescriptorBinding.h so
	// the convention lives in exactly one place instead of being duplicated across
	// material classes and shaders.
	namespace buffer_index {
		// Geometry vertex buffer, consumed through [[stage_in]]. The MTLVertexDescriptor
		// each material builds references this index in its layout.
		constexpr uint32_t k_VertexData = 0;

		// Scene globals. Vertex stage reads the view-projection; fragment stage reads
		// view-projection + camera position + directional light. Kept high so it never
		// collides with the stage_in vertex buffer at index 0. 30 is the highest index
		// Metal permits (the buffer argument table is 0..30), so the per-draw buffers
		// below descend from here rather than continuing upward past the limit.
		constexpr uint32_t k_Scene = 30;

		// Per-draw push data. Vertex stage: model matrix. Fragment stage: material
		// parameters (Phong shininess, or PBR albedo color + metalness + roughness).
		constexpr uint32_t k_Push = 29;

		// Point-light array (count header + PointLight[]), PBR fragment stage only.
		constexpr uint32_t k_Lights = 28;
	} // namespace buffer_index

} // namespace cbk::platform::metal
