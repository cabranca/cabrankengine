#include <pch.h>
#include "VulkanPBRMaterial.h"

#include "VulkanRendererAPI.h"
#include "VulkanTexture.h"

namespace cbk::platform::vk {

	using namespace rendering;

	VulkanPBRMaterial::VulkanPBRMaterial() {
		m_Device = VulkanRendererAPI::getContext().getDevice();
		m_DescriptorSet = VulkanRendererAPI::getPhongDescriptorSet();
	}

	void VulkanPBRMaterial::updateDescriptorSet() {
        if (m_DescriptorSetInitialized)
			return;
        

		auto albedoVk = static_cast<VulkanTexture*>(m_AlbedoMap.get());
		auto normalVk = static_cast<VulkanTexture*>(m_NormalMap.get());
		auto mrVk = static_cast<VulkanTexture*>(m_MetalRoughMap.get());
		auto aoVk = static_cast<VulkanTexture*>(m_AOMap.get());

		std::array<VkDescriptorImageInfo, 4> imageInfos{
			*albedoVk->getDescriptor(),
			*normalVk->getDescriptor(),
			*mrVk->getDescriptor(),
			*aoVk->getDescriptor(),
		};

		std::array<VkWriteDescriptorSet, 4> writes{};
		for (uint32_t i = 0; i < 4; ++i) {
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

} // namespace cbk::platform::vk
