#pragma once

namespace cbk::rendering {

	class StorageBuffer {
	  public:
		virtual ~StorageBuffer() = default;

		virtual void setData(const void* data, uint32_t size) = 0;
		virtual void bind(uint32_t bindingPoint) const = 0;

		static Ref<StorageBuffer> create(uint32_t size);
	};
} // namespace cbk::rendering
