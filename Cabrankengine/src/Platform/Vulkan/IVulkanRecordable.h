#pragma once

#include <volk/volk.h>

#include <Common/Math/Mat4.h>

namespace cbk::platform::vk {

	// Capability interface for materials that know how to record themselves into a
	// Vulkan command buffer. Lives as a sibling of rendering::Material — concrete
	// Vulkan materials multi-inherit from both, and VulkanRendererAPI dynamic_casts
	// to this interface when issuing draws.
	class IVulkanRecordable {
	  public:
		virtual ~IVulkanRecordable() = default;

		// Records everything needed to draw with this material into cb: binds the
		// pipeline, every descriptor set the material's layout declares (scene globals,
		// the material's own set, and — for lit materials — the light SSBO), and any
		// push constants. Called by VulkanRendererAPI between vkCmdBeginRendering and
		// vkCmdDrawIndexed. transform is the per-draw model matrix; materials that batch
		// in world space (Text, Texture2D) ignore it.
		//
		// The set-index convention is centralised in VulkanDescriptorBinding.h so it is
		// not duplicated across concrete materials.
		virtual void record(VkCommandBuffer cb, const math::Mat4& transform) const = 0;
	};

} // namespace cbk::platform::vk
