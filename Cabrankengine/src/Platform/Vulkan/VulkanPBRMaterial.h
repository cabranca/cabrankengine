#pragma once

#include <volk/volk.h>

#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>

namespace cbk::platform::vk {

	class VulkanPBRMaterial : public rendering::PBRMaterial {
	  public:
		VulkanPBRMaterial();

        void updateDescriptorSet();

	  private:
		bool m_DescriptorSetInitialized = false;

		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkDescriptorSet m_DescriptorSet{ VK_NULL_HANDLE }; // NON-OWNING
	};

} // namespace cbk::platform::vk
