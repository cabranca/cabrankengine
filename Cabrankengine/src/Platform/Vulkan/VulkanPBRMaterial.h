#pragma once

#include <volk/volk.h>

#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>

#include "IVulkanRecordable.h"

namespace cbk::platform::vk {

	class VulkanPBRMaterial : public rendering::PBRMaterial, public IVulkanRecordable {
	  public:
		VulkanPBRMaterial();
		~VulkanPBRMaterial() override;

		// No-op: state lives in the descriptor set and push constants. The actual
		// recording happens in recordCommandBuffer() during commitRenderCommands().
		void bind() const override {}

		// IVulkanRecordable
		[[nodiscard]] VkPipeline getPipeline() const override {
			return s_Pipeline;
		}
		[[nodiscard]] VkPipelineLayout getPipelineLayout() const override {
			return s_PipelineLayout;
		}
		[[nodiscard]] const VkDescriptorSet* getDescriptorSet() const override {
			return &m_DescriptorSet;
		}
		void recordCommandBuffer(VkCommandBuffer cb, VkPipelineLayout layout) const override;

		// Per-class pipeline state cleanup. Called by VulkanRendererAPI::shutdown().
		static void destroySharedResources();

	  private:
		struct PushData {
			math::Vector3 albedoColor;
			float         metalness;
			float         roughness;
		};

		static void initSharedResourcesIfNeeded();
		void        updateDescriptorSet() const;

		static bool                  s_Initialized;
		static VkDescriptorSetLayout s_DescriptorSetLayout;
		static VkDescriptorPool      s_DescriptorPool;
		static VkPipelineLayout      s_PipelineLayout;
		static VkPipeline            s_Pipeline;

		VkDescriptorSet m_DescriptorSet{ VK_NULL_HANDLE };
		mutable bool    m_DescriptorDirty{ true };
	};

} // namespace cbk::platform::vk
