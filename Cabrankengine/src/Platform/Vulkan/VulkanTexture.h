#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Renderer/Texture.h>

namespace cbk::platform::vk {

	class VulkanTexture : public rendering::Texture2D {
	  public:
		VulkanTexture(const rendering::TextureSpecification& specification);
		VulkanTexture(const std::string& path);
		VulkanTexture(const FT_Face& face);

		~VulkanTexture();

		// Returns the specification of the texture, which includes its width, height, format, and mip generation status.
		const rendering::TextureSpecification& getSpecification() const override;

		// Returns the texture width
		uint32_t getWidth() const override;

		// Returns the texture height
		uint32_t getHeight() const override;

		// Returns the texture's renderer ID, which is used by the graphics API to identify the texture.
		uint64_t getRendererID() const override;

		// Returns the path to the texture file.
		const std::string& getPath() const override;

		// Sets the texture data from a raw data pointer and size.
		void setData(void* data, uint32_t size) override;

		// Binds the texture to a specific slot in the graphics API, allowing it to be used in rendering.
		void bind(uint32_t slot = 0) const override;

		// Returns whether the texture is loaded successfully.
		bool isLoaded() const override;

		// Returns whether both textures are equal based on their specifications and renderer IDs.
		virtual bool operator==(const Texture& other) const override;

		[[nodiscard]] const VkDescriptorImageInfo* getDescriptor() const;

	  private:
		// One entry per mip level in the staging buffer. Single-level callers
		// (font glyphs, 1x1 fallbacks, runtime setData) pass a 1-element vector;
		// the .cbkt loader passes the full chain.
		struct MipLevel {
			uint32_t width;
			uint32_t height;
			VkDeviceSize byteOffset;
			VkDeviceSize byteSize;
		};

		// Creates the GPU image + view + sampler sized m_Width x m_Height with
		// `mips.size()` levels, uploads each level via a single staging buffer,
		// transitions all levels to shader-read layout, and populates
		// m_TexDescriptorInfo. Shared by all three constructors / setData.
		void uploadPixels(VkFormat format, const void* data, VkDeviceSize dataSize,
		                  const std::vector<MipLevel>& mips);

		struct TextureData {
			VmaAllocation allocation{ VK_NULL_HANDLE };
			VkImage image{ VK_NULL_HANDLE };
			VkImageView view{ VK_NULL_HANDLE };
			VkSampler sampler{ VK_NULL_HANDLE };
		};
		
		TextureData m_Texture;
		VkDescriptorImageInfo m_TexDescriptorInfo{};

		rendering::TextureSpecification
		    m_Specification; // Specification of the texture, including width, height, format, and mip generation status

		std::string m_Path;         // Path to the texture file, used for loading the texture from disk
		bool m_IsLoaded = false;    // Flag indicating whether the texture has been loaded successfully
		uint32_t m_Width, m_Height; // Width and height of the texture, used for rendering and mip generation
	};
} // namespace cbk::platform::vk