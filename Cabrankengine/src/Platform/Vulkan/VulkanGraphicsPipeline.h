#pragma once

#include <volk/volk.h>

#include <Common/Math/Mat4.h>

#include "VulkanDescriptorBuffer.h"

namespace cbk::platform::vk {

	// Hardcoded struct for every graphics pipeline (Phong and PBR). Mirrors the
	// ConstantBuffer<SceneData> block in Phong.slang: this is a memory layout, not a
	// domain type. std140 rounds every vec3 up to a 16-byte slot, so the pads are
	// load-bearing and must stay in step with the shader's _pad0/_pad1/_pad2. Do not
	// substitute rendering::DirectionalLight here -- it is tightly packed.
	struct SceneData {
		struct DirLightData {
			math::Vector3 direction{ 0.f, -1.f, 0.f };
			float _Pad0 = 0.0f;
			// Defaults to zero radiance: a scene with no CDirectionalLight authored
			// gets no directional light, rather than a phantom white sun.
			math::Vector3 radiance{ 0.f };
			float _Pad1 = 0.0f;
		};

		math::Mat4 ViewProjectionMatrix; // 64 bytes (offset 0)
		DirLightData DirLight;           // 32 bytes (offset 64)
		math::Vector3 CameraPosition;    // 12 bytes (offset 96)
		float _Pad2 = 0.0f;              //  4 bytes (offset 108 -> 112 total)
	};
	static_assert(sizeof(SceneData) == 112, "SceneData must match the std140 layout of Phong.slang's SceneData");

	// For now it's a Phong Graphics Pipeline.
	class VulkanGraphicsPipeline {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
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
