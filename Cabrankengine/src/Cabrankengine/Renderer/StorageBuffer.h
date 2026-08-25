#pragma once

#include <cstdint>

namespace cbk::rendering {

	class StorageBuffer {
	  public:
		virtual ~StorageBuffer() = default;

		virtual void setData(uint32_t frameIndex, const void* data, uint32_t size) = 0;

		// [[nodiscard]] static Ref<StorageBuffer> create(uint32_t size);
	};
} // namespace cbk::rendering
