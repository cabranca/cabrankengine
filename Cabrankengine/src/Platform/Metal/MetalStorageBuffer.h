#pragma once

#include <vector>

#include <Cabrankengine/Renderer/StorageBuffer.h>

namespace cbk::platform::metal {

	// The only storage buffer in the engine is the per-frame point-light array
	// (count header + up to 64 PointLight = 3088 bytes), which is under Metal's 4 KB
	// setBytes limit. Rather than allocate an MTL::Buffer, this just retains the
	// latest bytes on the CPU; MetalPBRMaterial::record() pushes them onto the
	// encoder via setFragmentBytes. bind() is therefore a no-op.
	class MetalStorageBuffer : public rendering::StorageBuffer {
	  public:
		MetalStorageBuffer(uint32_t size);
		~MetalStorageBuffer() override;
		void setData(const void* data, uint32_t size) override;
		void bind(uint32_t bindingPoint) const override;

		[[nodiscard]] const void* data() const {
			return m_Data.data();
		}
		[[nodiscard]] uint32_t size() const {
			return static_cast<uint32_t>(m_Data.size());
		}

	  private:
		std::vector<uint8_t> m_Data;
	};
} // namespace cbk::platform::metal
