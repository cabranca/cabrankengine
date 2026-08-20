#pragma once

#include <volk/volk.h>

#include <Cabrankengine/Renderer/Renderer.h>

#include "VulkanStorageBuffer.h"
#include "VulkanUniformBuffer.h"

namespace cbk::platform::vk {

	// Descriptor-set index convention shared by every Vulkan material pipeline.
	// Set 0 and set 2 hold renderer-owned frame globals; set 1 is each material's
	// own per-instance descriptor. Materials bind set 1 themselves inside record();
	// these helpers centralise the globals so the index convention lives in exactly
	// one place instead of being duplicated across material classes.
	namespace set_index {
		constexpr uint32_t k_Scene = 0;    // scene globals UBO (view-proj, dir light, camera)
		constexpr uint32_t k_Material = 1; // per-material descriptor (textures)
		constexpr uint32_t k_Light = 2;    // point-light SSBO
	} // namespace set_index

	// Binds set 0 — the scene globals UBO owned by the Renderer. Every material
	// pipeline declares this set, so every material's record() calls it.
	// inline void bindSceneSet(VkCommandBuffer cb, VkPipelineLayout layout) {
	// 	auto sceneUbo = static_cast<VulkanUniformBuffer*>(rendering::Renderer::getSceneUBO().get());
	// 	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set_index::k_Scene, 1, sceneUbo->getDescriptorSet(), 0,
	// 	                        nullptr);
	// }

	// Binds set 2 — the point-light SSBO owned by the Renderer. Only materials whose
	// pipeline layout declares set 2 (currently just PBR) call this.
	inline void bindLightSet(VkCommandBuffer cb, VkPipelineLayout layout) {
		auto lightSSBO = static_cast<VulkanStorageBuffer*>(rendering::Renderer::getLightSSBO().get());
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set_index::k_Light, 1, lightSSBO->getDescriptorSet(), 0,
		                        nullptr);
	}

} // namespace cbk::platform::vk
