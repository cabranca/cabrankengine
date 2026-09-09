#pragma once

#include "VulkanGraphicsPipeline.h"

namespace cbk::platform::vk {

	class VulkanPBRGraphicsPipeline {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
		void shutdown();

		void bind(VkCommandBuffer cb, uint32_t frameIndex, VkDescriptorSet materialSet, const math::Mat4& transform,
		          const math::Vector3& albedoColor, float metalness, float roughness);

		[[nodiscard]] VkDescriptorSet allocateDescriptorSet();

	  private:
		// Mirrors PushConstants in PBR.slang. albedoColor is a float3 at offset 64, so
		// metalness packs into the tail of that same 16-byte row rather than starting a
		// new one — hence 84 and not 96.
		struct PushData {
			math::Mat4 transform;      // bytes  0..63
			math::Vector3 albedoColor; // bytes 64..75
			float metalness;           // bytes 76..79
			float roughness;           // bytes 80..83
		};
		static_assert(sizeof(PushData) == 84, "PushData must match the push-constant block of PBR.slang");

		VulkanGraphicsPipeline m_Pipeline;
	};
} // namespace cbk::platform::vk