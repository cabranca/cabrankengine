#include <pch.h>
#include "VulkanPhongGraphicsPipeline.h"

#include "VulkanPipelineHelpers.h"

namespace cbk::platform::vk {

	static constexpr uint32_t k_MaterialBindingCount = 2;
	static constexpr uint32_t k_MaxInstances = 256;

	// TODO: repeated code with PBR, probably needs a base class
	void VulkanPhongGraphicsPipeline::init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat,
	                                       VkSampleCountFlagBits sampleCount) {
		const auto bindings = createSetLayoutBindings(k_MaterialBindingCount);
		const VkDescriptorPoolSize poolSize = createPoolSize(k_MaterialBindingCount * k_MaxInstances);
		const VkShaderModule shaderModule = createShaderModule("Phong");
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

	void VulkanPhongGraphicsPipeline::shutdown() {
		m_Pipeline.shutdown();
	}

	void VulkanPhongGraphicsPipeline::bind(VkCommandBuffer cb, uint32_t frameIndex, VkDescriptorSet materialSet,
	                                       const math::Mat4& transform, float shininess) {
		PushData pushData{ .transform = transform, .shininess = shininess };
		std::vector<uint8_t> pushDataBuffer;
		pushDataBuffer.resize(sizeof(PushData));
		memcpy(pushDataBuffer.data(), &pushData, sizeof(PushData));
		m_Pipeline.bind(cb, frameIndex, materialSet, pushDataBuffer);
	}

	VkDescriptorSet VulkanPhongGraphicsPipeline::allocateDescriptorSet() {
		return m_Pipeline.allocateDescriptorSet();
	}
} // namespace cbk::platform::vk