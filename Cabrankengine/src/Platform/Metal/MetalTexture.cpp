#include <pch.h>
#include "MetalTexture.h"

#include <Metal/Metal.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <lz4.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/RenderCommand.h>
#include "MetalDeviceContext.h"
#include "MetalRendererAPI.h"

namespace cbk::platform::metal {

	using namespace rendering;

	namespace {
		MTL::PixelFormat imageFormatToMetalPixelFormat(ImageFormat format) {
			switch (format) {
				case ImageFormat::RGBA8:
					return MTL::PixelFormatRGBA8Unorm;
				case ImageFormat::RGB8:
					return MTL::PixelFormatRGBA8Unorm; // Metal has no RGB8; pad to RGBA
				case ImageFormat::R8:
					return MTL::PixelFormatR8Unorm;
				default:
					CBK_CORE_ASSERT(false, "Unsupported image format for Metal!");
					return MTL::PixelFormatRGBA8Unorm;
			}
		}

		uint32_t bytesPerPixelForFormat(ImageFormat format) {
			switch (format) {
				case ImageFormat::RGBA8:
					return 4;
				case ImageFormat::RGB8:
					return 4; // padded to RGBA
				case ImageFormat::R8:
					return 1;
				default:
					return 4;
			}
		}
	} // namespace

	void MetalTexture2D::createTexture(uint32_t metalPixelFormat, uint32_t mipLevels) {
		auto* context = static_cast<MetalDeviceContext*>(Application::get().getWindow().getContext());
		MTL::Device* device = context->getDevice();

		MTL::TextureDescriptor* desc =
		    MTL::TextureDescriptor::texture2DDescriptor(static_cast<MTL::PixelFormat>(metalPixelFormat), m_Width, m_Height, false);
		desc->setMipmapLevelCount(std::max(1u, mipLevels));
		desc->setUsage(MTL::TextureUsageShaderRead);
		desc->setStorageMode(MTL::StorageModeShared);

		m_Texture = device->newTexture(desc);
		desc->release();

		m_RendererID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_Texture));
	}

	void MetalTexture2D::uploadPixels(uint32_t metalPixelFormat, uint32_t bytesPerPixel, const void* data,
	                                  const std::vector<MipLevel>& mips) {
		CBK_CORE_ASSERT(!mips.empty(), "MetalTexture2D::uploadPixels(): mip list must not be empty");
		m_BytesPerPixel = bytesPerPixel;
		createTexture(metalPixelFormat, static_cast<uint32_t>(mips.size()));

		const uint8_t* base = static_cast<const uint8_t*>(data);
		for (uint32_t level = 0; level < mips.size(); ++level) {
			const MipLevel& m = mips[level];
			MTL::Region region = MTL::Region::Make2D(0, 0, m.width, m.height);
			m_Texture->replaceRegion(region, level, base + m.byteOffset, m.width * bytesPerPixel);
		}
		m_IsLoaded = true;
	}

	MetalTexture2D::MetalTexture2D(const TextureSpecification& specification)
	    : m_Specification(specification), m_Width(specification.Width), m_Height(specification.Height) {
		CBK_PROFILE_FUNCTION();

		m_BytesPerPixel = bytesPerPixelForFormat(specification.Format);
		createTexture(imageFormatToMetalPixelFormat(specification.Format), 1);
		m_IsLoaded = (m_Texture != nullptr);
	}

	MetalTexture2D::MetalTexture2D(const std::string& path, bool sRGB) : m_Path(path) {
		CBK_PROFILE_FUNCTION();

		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode)) {
			CBK_CORE_ERROR("Cannot find the path {0} - Error: {1}", path, errorCode.message());
			return;
		}

		std::ifstream file(path, std::ios::binary);
		if (!file) {
			CBK_CORE_ERROR("Cannot open file {0}", path);
			return;
		}

		TextureHeader header;
		if (!file.read(reinterpret_cast<char*>(&header), sizeof(TextureHeader))) {
			CBK_CORE_ERROR("Cannot read the file {0}", path);
			return;
		}
		if (header.magic != 0x43424B54) { // "CBKT"
			CBK_CORE_ERROR("Wrong extension of file {0} - .cbkt expected!", path);
			return;
		}
		if (header.version != 2) {
			CBK_CORE_ERROR("Unsupported .cbkt version {0} in {1} - re-run CBKAssetConverter", header.version, path);
			return;
		}

		std::vector<uint8_t> compressedBuffer(header.compressedSize);
		if (!file.read(reinterpret_cast<char*>(compressedBuffer.data()), compressedBuffer.size())) {
			CBK_CORE_ERROR("Cannot read the file {0}", path);
			return;
		}

		std::vector<uint8_t> uncompressedBuffer(header.uncompressedSize);
		int result =
		    LZ4_decompress_safe(reinterpret_cast<const char*>(compressedBuffer.data()), reinterpret_cast<char*>(uncompressedBuffer.data()),
		                        header.compressedSize, header.uncompressedSize);
		if (result < 0) {
			CBK_CORE_ERROR("LZ4 decompression failed for {0}", path);
			return;
		}

		m_Width = header.width;
		m_Height = header.height;
		const uint32_t mipLevels = std::max(1u, header.mipLevels);

		// Build per-level offsets matching the converter's tight packing (dimensions
		// clamped to >= 1), mirroring VulkanTexture's loader.
		auto buildMipLevels = [&](uint32_t bytesPerPixel) {
			std::vector<MipLevel> mips;
			mips.reserve(mipLevels);
			uint32_t w = m_Width, h = m_Height;
			size_t offset = 0;
			for (uint32_t i = 0; i < mipLevels; i++) {
				const size_t size = static_cast<size_t>(w) * h * bytesPerPixel;
				mips.push_back({ w, h, offset, size });
				offset += size;
				w = std::max(1u, w / 2);
				h = std::max(1u, h / 2);
			}
			return mips;
		};

		if (header.channels == 3) {
			// Metal has no 24-bit RGB sampled format; expand each mip to RGBA8.
			auto srcMips = buildMipLevels(3);
			size_t totalRgba = 0;
			for (const auto& m: srcMips)
				totalRgba += (m.byteSize / 3) * 4;
			std::vector<uint8_t> rgba(totalRgba);

			std::vector<MipLevel> dstMips;
			dstMips.reserve(mipLevels);
			size_t dstOffset = 0;
			for (const auto& src: srcMips) {
				const size_t pixelCount = src.byteSize / 3;
				const uint8_t* srcPtr = uncompressedBuffer.data() + src.byteOffset;
				uint8_t* dstPtr = rgba.data() + dstOffset;
				for (size_t p = 0; p < pixelCount; p++) {
					dstPtr[p * 4 + 0] = srcPtr[p * 3 + 0];
					dstPtr[p * 4 + 1] = srcPtr[p * 3 + 1];
					dstPtr[p * 4 + 2] = srcPtr[p * 3 + 2];
					dstPtr[p * 4 + 3] = 255;
				}
				dstMips.push_back({ src.width, src.height, dstOffset, pixelCount * 4 });
				dstOffset += pixelCount * 4;
			}
			// Color textures use the sRGB format so the GPU decodes to linear on sample.
			uploadPixels(sRGB ? MTL::PixelFormatRGBA8Unorm_sRGB : MTL::PixelFormatRGBA8Unorm, 4, rgba.data(), dstMips);
		} else {
			MTL::PixelFormat format;
			uint32_t bpp;
			switch (header.channels) {
				case 1:
					format = MTL::PixelFormatR8Unorm;
					bpp = 1;
					break;
				case 2:
					format = MTL::PixelFormatRG8Unorm;
					bpp = 2;
					break;
				case 4:
					format = sRGB ? MTL::PixelFormatRGBA8Unorm_sRGB : MTL::PixelFormatRGBA8Unorm;
					bpp = 4;
					break;
				default:
					CBK_CORE_ERROR("MetalTexture2D(): unsupported channel count {0} in {1}", header.channels, path);
					return;
			}
			auto mips = buildMipLevels(bpp);
			uploadPixels(format, bpp, uncompressedBuffer.data(), mips);
		}
	}

	MetalTexture2D::MetalTexture2D(const FT_Face& face) {
		CBK_PROFILE_FUNCTION();

		// FreeType renders each glyph as an 8-bit coverage bitmap; upload it as a
		// single-channel R8 texture. Empty glyphs (e.g. space) have a zero-size
		// bitmap — clamp to 1x1 so newTexture gets a valid extent.
		const uint32_t glyphW = face->glyph->bitmap.width;
		const uint32_t glyphH = face->glyph->bitmap.rows;
		m_Width = glyphW > 0 ? glyphW : 1;
		m_Height = glyphH > 0 ? glyphH : 1;

		if (glyphW > 0 && glyphH > 0) {
			std::vector<MipLevel> mips{ { glyphW, glyphH, 0, static_cast<size_t>(glyphW) * glyphH } };
			uploadPixels(MTL::PixelFormatR8Unorm, 1, face->glyph->bitmap.buffer, mips);
		} else {
			const uint8_t empty = 0;
			std::vector<MipLevel> mips{ { 1, 1, 0, 1 } };
			uploadPixels(MTL::PixelFormatR8Unorm, 1, &empty, mips);
		}
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
			m_IsLoaded = true;
		}
	}

	void MetalTexture2D::bind(uint32_t slot) const {
		CBK_PROFILE_FUNCTION();

		// bind() is part of the cross-backend Texture2D interface, so it can't take the
		// encoder as a parameter; reach the in-flight encoder through the single
		// RendererAPI instance instead (same access path the ImGui layer uses).
		auto* rendererAPI = static_cast<MetalRendererAPI*>(RenderCommand::getRendererAPI());
		auto* encoder = rendererAPI->getCommandEncoder();
		if (encoder && m_Texture)
			encoder->setFragmentTexture(m_Texture, slot);
	}
} // namespace cbk::platform::metal
