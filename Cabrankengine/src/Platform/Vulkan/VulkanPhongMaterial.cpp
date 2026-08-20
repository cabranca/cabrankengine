#include <pch.h>
#include "VulkanPhongMaterial.h"

#include "VulkanRendererAPI.h"
#include "VulkanTexture.h"

namespace cbk::platform::vk {

	using namespace rendering;

	VulkanPhongMaterial::VulkanPhongMaterial() {		
		m_Device = VulkanRendererAPI::getContext().getDevice();
		m_DescriptorSet = VulkanRendererAPI::getPhongDescriptorSet();
	}

	void VulkanPhongMaterial::updateDescriptorSet() {
		if (m_DescriptorSetInitialized)
			return;

		// 1. Get the textures' descriptorsImageInfo
		if (!m_DiffuseMap || !m_SpecularMap) {
			// Defer descriptor write until both texture slots are populated.
			return;
		}

		auto diffuseVk = static_cast<VulkanTexture*>(m_DiffuseMap.get());
		auto specularVk = static_cast<VulkanTexture*>(m_SpecularMap.get());

		std::array<VkDescriptorImageInfo, 2> imageInfos{
			*diffuseVk->getDescriptor(),
			*specularVk->getDescriptor(),
		};

		// 2. Update the descriptor sets writing them.
		std::array<VkWriteDescriptorSet, 2> writes{};
		for (uint32_t i = 0; i < 2; ++i) {
			writes[i] = VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSet,
				.dstBinding = i,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos[i],
			};
		}
		vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

		m_DescriptorSetInitialized = true;
	}

	float VulkanPhongMaterial::getShininess() const {
		return m_Shininess;
	}
} // namespace cbk::platform::vk
