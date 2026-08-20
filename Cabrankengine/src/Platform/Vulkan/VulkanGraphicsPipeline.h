#pragma once

#include <volk/volk.h>

#include <Common/Math/Mat4.h>

#include "VulkanUniformBuffer.h"

namespace cbk::platform::vk {

	struct DirectionalLight {
		math::Vector3 direction{ 0.f, -1.f, 0.f };
		// Defaults to zero radiance: a scene with no CDirectionalLight authored
		// gets no directional light, rather than a phantom white sun.
		math::Vector3 radiance{ 0.f };
	};

    // Hardcoded struct for every graphics pipeline (Phong and PBR)
    struct SceneData {
		math::Mat4 ViewProjectionMatrix;     // 64 bytes
		DirectionalLight DirLight; 			 // 32 bytes (Offset 64)
		math::Vector3 CameraPosition;  		 // 12 bytes (Offset 96)
		float _Pad2 = 0.0f;            		 // 4 bytes (Offset 108 -> Total 112 bytes) TODO: check if Slang needs this
	};

	// For now it's a Phong Graphics Pipeline.
	class VulkanGraphicsPipeline {
	  public:
		void init(const VulkanDeviceContext& vkDeviceContext);
		void shutdown();

		void bind(VkCommandBuffer cb, const math::Mat4& transform, float shininess, uint32_t frameIndex);

		[[nodiscard]] VulkanUniformBuffer& getUBO();

		// I'm pretty sure I've got to provide a new allocate descriptor set for each new material instance.
		// Maybe I should have a vector of descriptor sets here, inserted when allocated and I give that pointer to the material?
		// Then I bind the specific descriptor set for each material call.
		[[nodiscard]] VkDescriptorSet getDescriptorSet() const;

	  private:
		// Claude says I'm exceeding limit of push data. I don't follow
		struct PushData {
			math::Mat4 transform;
			float shininess;
		};

        static constexpr uint32_t k_SceneDataBinding = 0;
		static constexpr uint32_t k_MaterialBindingCount = 2;
		static constexpr uint32_t k_MaxInstances = 256;
        VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VulkanUniformBuffer m_UBO;

		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSet();
		void createPipelineLayout();
		void createPipeline(VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
	};
} // namespace cbk::platform::vk
