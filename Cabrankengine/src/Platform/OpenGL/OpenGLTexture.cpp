#include <pch.h>
#include "OpenGLTexture.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <lz4.h>

namespace cbk::platform::opengl {

	using namespace rendering;

	namespace utils {

		static GLenum cbkImageFormatToGLDataFormat(ImageFormat format) {
			switch (format) {
				case ImageFormat::RGB8:
					return GL_RGB;
				case ImageFormat::RGBA8:
					return GL_RGBA;
			}

			CBK_CORE_ASSERT(false, "Invalid image format!");
			return 0;
		}

		static GLenum cbkImageFormatToGLInternalFormat(ImageFormat format) {
			switch (format) {
				case ImageFormat::RGB8:
					return GL_RGB8;
				case ImageFormat::RGBA8:
					return GL_RGBA8;
			}

			CBK_CORE_ASSERT(false, "Invalid image format!");
			return 0;
		}

	} // namespace utils

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification)
	    : m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height) {
		CBK_PROFILE_FUNCTION();

		m_DataFormat = utils::cbkImageFormatToGLDataFormat(m_Specification.Format);
		m_InternalFormat = utils::cbkImageFormatToGLInternalFormat(m_Specification.Format);

		// Spec-constructed textures (1x1 white fallback etc.) don't carry a mip
		// chain; allocate a single level and use plain linear filtering.
#ifdef CBK_OPENGL_ES
		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexStorage2D(GL_TEXTURE_2D, 1, m_InternalFormat, m_Width, m_Height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#else
		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, bool sRGB) : m_Path(path) {
		CBK_PROFILE_FUNCTION();

		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode)) {
			CBK_CORE_ERROR("Cannot find the path {0} - Error: {1}", path, errorCode.message());
			return;
		}

		const auto size = std::filesystem::file_size(path, errorCode);
		if (errorCode) {
			CBK_CORE_ERROR("Size check failure for file {0} - Error: {1}", path, errorCode.message());
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
		if (!file.read(reinterpret_cast<char*>(compressedBuffer.data()), compressedBuffer.size()))
			CBK_CORE_ERROR("Cannot read the file {0}", path);
		else {
			std::vector<uint8_t> uncompressedBuffer(header.uncompressedSize);
			int result =
			    LZ4_decompress_safe(reinterpret_cast<const char*>(compressedBuffer.data()),
			                        reinterpret_cast<char*>(uncompressedBuffer.data()), header.compressedSize, header.uncompressedSize);
			if (result < 0) {
				CBK_CORE_ERROR("LZ4 decompression failed for {0}", path);
				return;
			}

			m_Width = header.width;
			m_Height = header.height;

			// Color textures (diffuse/albedo) request an sRGB internal format so the
			// GPU decodes sRGB->linear on every sample (and filters/mips in linear
			// space). The transfer (data) format stays GL_RGB/GL_RGBA — sRGB affects
			// only how the stored bytes are interpreted, not the upload layout.
			GLenum internalFormat = 0, dataFormat = 0;
			if (header.channels == 4) {
				internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				dataFormat = GL_RGBA;
			} else if (header.channels == 3) {
				internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
				dataFormat = GL_RGB;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			CBK_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

			const uint32_t mipLevels = std::max(1u, header.mipLevels);

#ifdef CBK_OPENGL_ES
			glGenTextures(1, &m_RendererID);
			glBindTexture(GL_TEXTURE_2D, m_RendererID);
			glTexStorage2D(GL_TEXTURE_2D, mipLevels, internalFormat, m_Width, m_Height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#else
			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, mipLevels, internalFormat, m_Width, m_Height);

			// Trilinear sampling + anisotropic filtering eliminate the shimmer/
			// "crispy" minification aliasing visible in Sponza. GL_TEXTURE_MAX_ANISOTROPY
			// is core in 4.6 and ubiquitously available via EXT_texture_filter_anisotropic
			// on 4.5; the enum value is identical.
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			GLfloat maxAniso = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
			glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, std::min(16.0f, maxAniso));

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif

			// Our mip chain is tightly packed with no per-row padding. GL_UNPACK_ALIGNMENT
			// defaults to 4, which would corrupt the upload for the smallest RGB mips
			// where a row's byte count is not 4-aligned (e.g. W=2 RGB = 6 bytes).
			GLint prevAlignment = 4;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// Upload each mip level. Offsets are recomputed in the same way the
			// converter laid them out (tight packing, dimensions clamped to >= 1).
			uint32_t mipW = m_Width;
			uint32_t mipH = m_Height;
			size_t offset = 0;
			for (uint32_t level = 0; level < mipLevels; level++) {
#ifdef CBK_OPENGL_ES
				glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), 0, 0, mipW, mipH, dataFormat, GL_UNSIGNED_BYTE,
				                uncompressedBuffer.data() + offset);
#else
				glTextureSubImage2D(m_RendererID, static_cast<GLint>(level), 0, 0, mipW, mipH, dataFormat, GL_UNSIGNED_BYTE,
				                    uncompressedBuffer.data() + offset);
#endif
				offset += static_cast<size_t>(mipW) * mipH * header.channels;
				mipW = std::max(1u, mipW / 2);
				mipH = std::max(1u, mipH / 2);
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);

			m_IsLoaded = true;
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(const FT_Face& face) {
		// Whitespace and several control glyphs have a 0x0 bitmap. glTexStorage2D rejects
		// zero dimensions (WebGL spams a warning per glyph), so use a 1x1 placeholder —
		// metrics like advance/bearing are still meaningful for layout.
		const uint32_t width = std::max<uint32_t>(face->glyph->bitmap.width, 1);
		const uint32_t height = std::max<uint32_t>(face->glyph->bitmap.rows, 1);
		const bool hasPixels = face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0;

#ifdef CBK_OPENGL_ES
		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, width, height);
		if (hasPixels)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
		// generate texture
		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, GL_R8, width, height);
		if (hasPixels)
			glTextureSubImage2D(m_RendererID, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		// set texture options
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
	}

	OpenGLTexture2D::~OpenGLTexture2D() {
		CBK_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::setData(void* data, uint32_t size) {
		CBK_PROFILE_FUNCTION();

		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		CBK_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
#ifdef CBK_OPENGL_ES
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
#else
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
#endif
	}

	void OpenGLTexture2D::bind(uint32_t slot) const {
		CBK_PROFILE_FUNCTION();

#ifdef CBK_OPENGL_ES
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
#else
		glBindTextureUnit(slot, m_RendererID);
#endif
	}
} // namespace cbk::platform::opengl
