#pragma once

#include <volk/volk.h>

#include "VulkanUniformBuffer.h"

namespace cbk::platform::vk {

    // Hardcoded struct for every graphics pipeline (Phong and PBR)
    struct SceneData {
		Mat4 ViewProjectionMatrix;     // 64 bytes
		DirectionalLightData DirLight; // 32 bytes (Offset 64)
		math::Vector3 CameraPosition;  // 12 bytes (Offset 96)
		float _Pad2 = 0.0f;            // 4 bytes (Offset 108 -> Total 112 bytes) TODO: check if Slang needs this
	};

	class VulkanGraphicsPipeline {
	  public:
		void init(const VulkanDeviceContext& vkDeviceContext);
		void shutdown();

	  private:
        static constexpr uint32_t k_SceneDataBinding = 0;
        VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkGraphicsPipeline m_Pipeline = VK_NULL_HANDLE;
        VulkanUniformBuffer m_UBO;
	};
} // namespace cbk::platform::vk