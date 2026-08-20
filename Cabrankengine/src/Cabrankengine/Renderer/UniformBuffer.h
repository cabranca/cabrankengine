#pragma once

namespace cbk::rendering {

	class UniformBuffer {
	  public:
		virtual ~UniformBuffer() = default;

		virtual void setData(uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset = 0) = 0;

		//[[nodiscard]] static Ref<UniformBuffer> create(const GraphicsContext& ctx, uint32_t size, uint32_t binding, VkShaderStageFlags stageFlags);
	};
} // namespace cbk::rendering