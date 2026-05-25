#pragma once

#include <volk/volk.h>

namespace cbk::platform::vk {

	// Capability interface for materials that know how to record their state into
	// a Vulkan command buffer. Lives as a sibling of rendering::Material — concrete
	// Vulkan materials multi-inherit from both, and VulkanRendererAPI dynamic_casts
	// to this interface when issuing draws.
	class IVulkanRecordable {
	  public:
		virtual ~IVulkanRecordable() = default;

		// Pipeline + layout for this material. Shared across all instances of a
		// given concrete material class — exposed for VulkanRendererAPI to bind.
		[[nodiscard]] virtual VkPipeline getPipeline() const             = 0;
		[[nodiscard]] virtual VkPipelineLayout getPipelineLayout() const = 0;

		// Per-instance material descriptor set (bound at set index 1).
		[[nodiscard]] virtual const VkDescriptorSet* getDescriptorSet() const = 0;

		// Records material-specific commands (fragment push constants, dirty descriptor writes).
		// Called by VulkanRendererAPI after pipeline + descriptor set bind, before vkCmdDrawIndexed.
		virtual void recordCommandBuffer(VkCommandBuffer cb, VkPipelineLayout layout) const = 0;

		// Override to true on materials whose pipeline layout includes set 2 = point
		// light SSBO (currently just PBR). VulkanRendererAPI gates the set 2 bind on
		// this so Phong / Text / Texture2D pipelines — which don't declare it — don't
		// trip descriptor-set-compatibility validation errors.
		[[nodiscard]] virtual bool wantsLightSSBO() const {
			return false;
		}
	};

} // namespace cbk::platform::vk
