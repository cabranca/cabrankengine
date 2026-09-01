#pragma once

#include <volk/volk.h>

#include <Cabrankengine/Renderer/RendererAPI.h>
#include <Common/Math/Mat4.h>

#include "VulkanDescriptorBuffer.h"

namespace cbk::platform::vk {

	struct GPUDirLight {
		math::Vector3 Direction{ 0.f, -1.f, 0.f };
		float Pad0 = 0.0f;
		math::Vector3 Radiance{ 0.f };
		float Pad1 = 0.0f;
	};

	struct GPUPointLight {
		math::Vector4 Position; // x, y, z, padding
		math::Vector4 Radiance; // r, g, b, padding
		float Constant;
		float Linear;
		float Quadratic;
		float Padding; // To complete (16 * 3) bytes
	};

	struct GPUPointLightsBufferHeader {
		int Count;
		int Padding[3]; // Align with PointLightGPU
	};

	struct GPUCameraData {
		math::Mat4 ViewProjectionMatrix; // 64 bytes (offset 0)
		math::Vector3 CameraPosition;    // 12 bytes (offset 64)
		float Pad2 = 0.0f;               //  4 bytes (offset 76 -> total 80)
	};

	struct UBOData {
		GPUCameraData CameraData;		 // 80 bytes (offset 0)
		GPUDirLight DirLight;            // 32 bytes (offset 80 -> total 112)
	};
	static_assert(sizeof(UBOData) == 112, "SceneData must match the std140 layout of Phong.slang's SceneData");

	// For now it's a Phong Graphics Pipeline.
	class VulkanGraphicsPipeline {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
		void shutdown();

		void setSceneData(const rendering::SceneData& sceneData, uint32_t frameIndex);
		void bind(VkCommandBuffer cb, const math::Mat4& transform, float shininess, uint32_t frameIndex);

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
		VulkanStorageBuffer m_SSBO;

		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSet();
		void createPipelineLayout();
		void createPipeline(VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
		void setUBOData(uint32_t frameIndex, math::Mat4 viewProjectionMatrix, math::Vector3 cameraPosition, rendering::DirectionalLight dirLight);
		void setSSBOData(uint32_t frameIndex, const std::vector<rendering::PointLight>& pointLights);
	};
} // namespace cbk::platform::vk
