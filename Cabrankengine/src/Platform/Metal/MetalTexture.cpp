#include <pch.h>
#include "MetalTexture.h"

#include <Metal/Metal.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include "MetalContext.h"
#include "MetalRendererAPI.h"

namespace cbk::platform::metal {

	using namespace rendering;

	static MTL::PixelFormat imageFormatToMetalPixelFormat(ImageFormat format) {
		switch (format) {
			case ImageFormat::RGBA8: return MTL::PixelFormatRGBA8Unorm;
			case ImageFormat::RGB8:  return MTL::PixelFormatRGBA8Unorm; // Metal has no RGB8; we use RGBA8 and pad
			case ImageFormat::R8:    return MTL::PixelFormatR8Unorm;
			default:
				CBK_CORE_ASSERT(false, "Unsupported image format for Metal!");
				return MTL::PixelFormatRGBA8Unorm;
		}
	}

	static uint32_t bytesPerPixelForFormat(ImageFormat format) {
		switch (format) {
			case ImageFormat::RGBA8: return 4;
			case ImageFormat::RGB8:  return 4; // We pad to RGBA
			case ImageFormat::R8:    return 1;
			default: return 4;
		}
	}

	void MetalTexture2D::createTexture(uint32_t width, uint32_t height, ImageFormat format) {
		// auto* context = static_cast<MetalContext*>(Application::get().getWindow().getContext());
		// MTL::Device* device = context->getDevice();

		// MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
		// 	imageFormatToMetalPixelFormat(format),
		// 	width, height,
		// 	false // no mipmaps for now
		// );
		// desc->setUsage(MTL::TextureUsageShaderRead);
		// desc->setStorageMode(MTL::StorageModeShared);

		// m_Texture = device->newTexture(desc);
		// desc->release();
	}

	MetalTexture2D::MetalTexture2D(const TextureSpecification& specification)
		: m_Specification(specification), m_Width(specification.Width), m_Height(specification.Height) {
		// CBK_PROFILE_FUNCTION();

		// m_BytesPerPixel = bytesPerPixelForFormat(specification.Format);
		// createTexture(m_Width, m_Height, specification.Format);
		// m_IsLoaded = (m_Texture != nullptr);
	}

	MetalTexture2D::MetalTexture2D(const std::string& path) : m_Path(path) {
		CBK_PROFILE_FUNCTION();

		// int width, height, channels;
		// stbi_set_flip_vertically_on_load(1);
		// stbi_uc* data = nullptr;

		// {
		// 	CBK_PROFILE_FUNCTION();
		// 	data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		// }

		// if (data) {
		// 	m_IsLoaded = true;
		// 	m_Width = width;
		// 	m_Height = height;

		// 	ImageFormat format = (channels == 4) ? ImageFormat::RGBA8 : ImageFormat::RGBA8;
		// 	m_BytesPerPixel = 4;
		// 	createTexture(m_Width, m_Height, format);

		// 	// If source is RGB (3 channels), we need to pad each pixel to RGBA
		// 	if (channels == 3) {
		// 		std::vector<uint8_t> rgbaData(m_Width * m_Height * 4);
		// 		for (uint32_t i = 0; i < m_Width * m_Height; i++) {
		// 			rgbaData[i * 4 + 0] = data[i * 3 + 0];
		// 			rgbaData[i * 4 + 1] = data[i * 3 + 1];
		// 			rgbaData[i * 4 + 2] = data[i * 3 + 2];
		// 			rgbaData[i * 4 + 3] = 255;
		// 		}
		// 		MTL::Region region = MTL::Region::Make2D(0, 0, m_Width, m_Height);
		// 		m_Texture->replaceRegion(region, 0, rgbaData.data(), m_Width * 4);
		// 	} else {
		// 		MTL::Region region = MTL::Region::Make2D(0, 0, m_Width, m_Height);
		// 		m_Texture->replaceRegion(region, 0, data, m_Width * m_BytesPerPixel);
		// 	}

		// 	stbi_image_free(data);
		// }
	}

	MetalTexture2D::MetalTexture2D(const FT_Face& face) {
		CBK_PROFILE_FUNCTION();

		// m_Width = face->glyph->bitmap.width;
		// m_Height = face->glyph->bitmap.rows;
		// m_BytesPerPixel = 1;

		// createTexture(m_Width, m_Height, ImageFormat::R8);

		// if (m_Texture && m_Width > 0 && m_Height > 0) {
		// 	MTL::Region region = MTL::Region::Make2D(0, 0, m_Width, m_Height);
		// 	m_Texture->replaceRegion(region, 0, face->glyph->bitmap.buffer, m_Width);
		// }
	}

	MetalTexture2D::~MetalTexture2D() {
		CBK_PROFILE_FUNCTION();

		if (m_Texture)
			m_Texture->release();
	}

	void MetalTexture2D::setData(void* data, uint32_t size) {
		CBK_PROFILE_FUNCTION();

		CBK_CORE_ASSERT(size == m_Width * m_Height * m_BytesPerPixel, "Data must be entire texture!");

		if (m_Texture) {
			MTL::Region region = MTL::Region::Make2D(0, 0, m_Width, m_Height);
			m_Texture->replaceRegion(region, 0, data, m_Width * m_BytesPerPixel);
		}
	}

	void MetalTexture2D::bind(uint32_t slot) const {
		CBK_PROFILE_FUNCTION();

		auto* encoder = MetalRendererAPI::GetActiveEncoder();
		if (encoder && m_Texture)
			encoder->setFragmentTexture(m_Texture, slot);
	}
}
