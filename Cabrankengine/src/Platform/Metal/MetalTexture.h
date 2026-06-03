#pragma once

#include <Cabrankengine/Renderer/Texture.h>

namespace MTL {
	class Texture;
}

namespace cbk::platform::metal {

	class MetalTexture2D : public rendering::Texture2D {
		public:
			MetalTexture2D(const rendering::TextureSpecification& specification);
			MetalTexture2D(const std::string& path);
			MetalTexture2D(const FT_Face& face);
			~MetalTexture2D();

			// Returns the specification of the texture, which includes its width, height, format, and mip generation status.
			const rendering::TextureSpecification& getSpecification() const override { return m_Specification; }

			// Returns the texture width
			uint32_t getWidth() const override { return m_Width; }
		
			// Returns the texture height
			uint32_t getHeight() const override { return m_Height; }
		
			// Returns the texture's renderer ID, which is used by the graphics API to identify the texture.
			uint64_t getRendererID() const override { return m_RendererID; }

			// Returns the path to the texture file.
			const std::string& getPath() const override { return m_Path; }

			// Sets the texture data from a raw data pointer and size.
			void setData(void* data, uint32_t size) override;

			// Binds the texture to a specific slot in the graphics API, allowing it to be used in rendering.
			void bind(uint32_t slot = 0) const override;

			// Returns whether the texture is loaded successfully.
			bool isLoaded() const override { return m_IsLoaded; }

			// Returns whether both textures are equal based on their specifications and renderer IDs.
			bool operator==(const Texture& other) const override {
				return m_RendererID == other.getRendererID();
			}
		private:
			void createTexture(uint32_t width, uint32_t height, rendering::ImageFormat format);

			rendering::TextureSpecification m_Specification;

			std::string m_Path;
			bool m_IsLoaded = false;
			uint32_t m_Width = 0, m_Height = 0;
			uint32_t m_RendererID = 0; // Kept for interface compatibility (operator==)
			uint32_t m_BytesPerPixel = 4;

			MTL::Texture* m_Texture = nullptr;
	};

}
