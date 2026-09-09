#pragma once

#include <vector>

#include <volk/volk.h>

#include "VulkanShader.h"

namespace cbk::platform::vk {

    inline std::vector<VkDescriptorSetLayoutBinding> createSetLayoutBindings(uint32_t bindingCount) {
		std::vector<VkDescriptorSetLayoutBinding> bindings{bindingCount};
		for (uint32_t i = 0; i < bindingCount; ++i) {
			bindings[i] = VkDescriptorSetLayoutBinding{
				.binding = i,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			};
		}
		return bindings;
	}

	inline VkDescriptorPoolSize createPoolSize(uint32_t descriptorCount) {
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = descriptorCount,
		};
		return poolSize;
	}

	inline VkShaderModule createShaderModule(std::string_view shaderName) {
		rendering::ShaderLibrary::load("assets/shaders/" + std::string(shaderName));
		return static_cast<VulkanShader*>(rendering::ShaderLibrary::get(shaderName).get())->getModule();
	}
}