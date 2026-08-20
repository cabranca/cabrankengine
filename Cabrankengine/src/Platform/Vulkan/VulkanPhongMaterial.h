#pragma once

#include <volk/volk.h>

#include <Cabrankengine/Renderer/Materials/PhongMaterial.h>

namespace cbk::platform::vk {

	class VulkanPhongMaterial : public rendering::PhongMaterial {
	  public:
		VulkanPhongMaterial();

		void updateDescriptorSet();

		[[nodiscard]] float getShininess() const;

	  private:
		bool m_DescriptorSetInitialized = false;

		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkDescriptorSet m_DescriptorSet{ VK_NULL_HANDLE }; // NON-OWNING
	};

} // namespace cbk::platform::vk
