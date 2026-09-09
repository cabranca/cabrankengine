#pragma once

#include <volk/volk.h>

#include <Cabrankengine/Renderer/RendererAPI.h>
#include <Common/Math/Mat4.h>

#include "VulkanDescriptorBuffer.h"

namespace cbk::platform::vk {

	struct PipelineDescriptor {
		VkDevice device = VK_NULL_HANDLE;
		VmaAllocator allocator = VK_NULL_HANDLE;
		VkFormat colorFormat;
		VkFormat depthFormat;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
		std::span<const VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorPoolSize poolSize;
		uint32_t maxSets = 0;
		uint32_t pushConstantsRangeSize = 0;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
	};

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
		uint32_t Count;
		uint32_t Padding[3]; // Align with PointLightGPU
	};

	struct GPUCameraData {
		math::Mat4 ViewProjectionMatrix; // 64 bytes (offset 0)
		math::Vector3 CameraPosition;    // 12 bytes (offset 64)
		float Pad2 = 0.0f;               //  4 bytes (offset 76 -> total 80)
	};

	struct UBOData {
		GPUCameraData CameraData; // 80 bytes (offset 0)
		GPUDirLight DirLight;     // 32 bytes (offset 80 -> total 112)
	};
	static_assert(sizeof(UBOData) == 112, "SceneData must match the std140 layout of Phong.slang's SceneData");

	class VulkanGraphicsPipeline {
	  public:
		// The scene UBO and light SSBO are shared by every pipeline — same data, same
		// layouts — so they are owned once here rather than per instance. Must run before
		// any init(), which reads their set layouts to build the pipeline layout, and
		// shutdownSceneResources() must run after every shutdown().
		static void initSceneResources(VkDevice device, VmaAllocator allocator);
		static void shutdownSceneResources();

		// Uploads set 0 and set 2 for the frame. Static because it only touches the shared
		// buffers: one call per frame covers every pipeline.
		static void setSceneData(const rendering::SceneData& sceneData, uint32_t frameIndex);

		void init(const PipelineDescriptor& pipelineDesc);
		void shutdown();
		void bind(VkCommandBuffer cb, uint32_t frameIndex, VkDescriptorSet materialSet, const std::vector<uint8_t>& pushConstants);

		// Set 1 is per-material-instance, not per-pipeline: every material owns the textures
		// it writes, so each one gets its own set out of this pipeline's pool. Sharing a
		// single set meant the last material to write it won for the whole frame, and the
		// write raced command buffers still in flight.
		[[nodiscard]] VkDescriptorSet allocateDescriptorSet();

	  private:
		static constexpr uint32_t k_SceneDataBinding = 0;
		static constexpr uint32_t k_PointLightsBinding = 0;
		static constexpr uint32_t k_MaxPointLights = 10;

		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		inline static VulkanUniformBuffer s_UBO;
		inline static VulkanStorageBuffer s_SSBO;

		void createDescriptorSetLayout(std::span<const VkDescriptorSetLayoutBinding> bindings);
		void createDescriptorPool(VkDescriptorPoolSize poolSize, uint32_t maxSets);
		void createPipelineLayout(uint32_t pushConstantsRangeSize);
		void createPipeline(VkShaderModule shaderModule, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
		static void setUBOData(uint32_t frameIndex, math::Mat4 viewProjectionMatrix, math::Vector3 cameraPosition,
		                       rendering::DirectionalLight dirLight);
		static void setSSBOData(uint32_t frameIndex, const std::vector<rendering::PointLight>& pointLights);
	};
} // namespace cbk::platform::vk
