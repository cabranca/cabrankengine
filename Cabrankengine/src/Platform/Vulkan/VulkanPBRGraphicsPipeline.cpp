#include <pch.h>
#include "VulkanPBRGraphicsPipeline.h"

#include "VulkanPipelineHelpers.h"

namespace cbk::platform::vk {

	static constexpr uint32_t k_MaterialBindingCount = 4;
	static constexpr uint32_t k_MaxInstances = 256;

	// TODO: repeated code with Phong, probably needs a base class
	void VulkanPBRGraphicsPipeline::init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat,
	                                     VkSampleCountFlagBits sampleCount) {
		const auto bindings = createSetLayoutBindings(k_MaterialBindingCount);
		const VkDescriptorPoolSize poolSize = createPoolSize(k_MaterialBindingCount * k_MaxInstances);
		const VkShaderModule shaderModule = createShaderModule("PBR");
		m_Pipeline.init({ .device = device,
		                  .allocator = allocator,
		                  .colorFormat = colorFormat,
		                  .depthFormat = depthFormat,
		                  .sampleCount = sampleCount,
		                  .bindings = { bindings },
		                  .poolSize = poolSize,
		                  .maxSets = k_MaxInstances,
		                  .pushConstantsRangeSize = sizeof(PushData),
		                  .shaderModule = shaderModule });
	}

	void VulkanPBRGraphicsPipeline::shutdown() {
		m_Pipeline.shutdown();
	}

	void VulkanPBRGraphicsPipeline::bind(VkCommandBuffer cb, uint32_t frameIndex, VkDescriptorSet materialSet, const math::Mat4& transform,
	                                     const math::Vector3& albedoColor, float metalness, float roughness) {
		PushData pushData{ .transform = transform, .albedoColor = albedoColor, .metalness = metalness, .roughness = roughness };
		std::vector<uint8_t> pushDataBuffer;
		pushDataBuffer.resize(sizeof(PushData));
		memcpy(pushDataBuffer.data(), &pushData, sizeof(PushData));
		m_Pipeline.bind(cb, frameIndex, materialSet, pushDataBuffer);
	}

	VkDescriptorSet VulkanPBRGraphicsPipeline::allocateDescriptorSet() {
		return m_Pipeline.allocateDescriptorSet();
	}
} // namespace cbk::platform::vk