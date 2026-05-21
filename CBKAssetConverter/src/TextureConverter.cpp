#include "TextureConverter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

#include <Common/Logger.h>
#include <lz4.h>
#include <stb_image.h>

namespace cbk::ac {

	uint32_t TextureConverter::s_MaxDimension = 0;

	void TextureConverter::setMaxDimension(uint32_t maxDim) {
		s_MaxDimension = maxDim;
	}

	namespace {
		// Box-filter downscale of an interleaved 8-bit image so neither dimension
		// exceeds maxDim. Returns an empty vector when no resize is needed (the
		// caller then keeps the original pixels); otherwise width/height are updated.
		std::vector<uint8_t> downscaleToFit(const uint8_t* src, int& width, int& height,
		                                    int channels, uint32_t maxDim) {
			if (maxDim == 0 || (width <= static_cast<int>(maxDim) && height <= static_cast<int>(maxDim)))
				return {};

			const float scale = std::min(static_cast<float>(maxDim) / width,
			                             static_cast<float>(maxDim) / height);
			const int dstW = std::max(1, static_cast<int>(width * scale));
			const int dstH = std::max(1, static_cast<int>(height * scale));

			std::vector<uint8_t> dst(static_cast<size_t>(dstW) * dstH * channels);
			for (int y = 0; y < dstH; y++) {
				const int sy0 = y * height / dstH;
				const int sy1 = std::max(sy0 + 1, (y + 1) * height / dstH);
				for (int x = 0; x < dstW; x++) {
					const int sx0 = x * width / dstW;
					const int sx1 = std::max(sx0 + 1, (x + 1) * width / dstW);
					for (int c = 0; c < channels; c++) {
						uint32_t sum = 0, n = 0;
						for (int sy = sy0; sy < sy1; sy++)
							for (int sx = sx0; sx < sx1; sx++) {
								sum += src[(static_cast<size_t>(sy) * width + sx) * channels + c];
								n++;
							}
						dst[(static_cast<size_t>(y) * dstW + x) * channels + c] =
						    static_cast<uint8_t>(sum / n);
					}
				}
			}
			width = dstW;
			height = dstH;
			return dst;
		}
	} // namespace

	void TextureConverter::convert(std::string_view path) {
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = nullptr;

		data = stbi_load(path.data(), &width, &height, &channels, 0);

		if (!data) {
			CBK_AC_ERROR("Failed to load texture: {}", path);
			return;
		}

		std::vector<uint8_t> resized = downscaleToFit(data, width, height, channels, s_MaxDimension);
		const uint8_t* pixels = resized.empty() ? data : resized.data();

		uint32_t dataSize = static_cast<uint32_t>(width) * height * channels;

		int compressedCapacity = LZ4_compressBound(dataSize);
		std::vector<char> compressedData(compressedCapacity);
		int compressedSize = LZ4_compress_default(reinterpret_cast<const char*>(pixels), compressedData.data(), dataSize, compressedCapacity);

		if (compressedSize <= 0) {
			CBK_AC_ERROR("LZ4 compression failed for: {}", path);
			stbi_image_free(data);
			return;
		}

		TextureHeader header{ .width = static_cast<uint32_t>(width),
			                 .height = static_cast<uint32_t>(height),
			                 .channels = static_cast<uint32_t>(channels),
			                 .compressedSize = static_cast<uint32_t>(compressedSize),
			                 .uncompressedSize = dataSize };

		std::filesystem::path outputPath(path);
		outputPath.replace_extension(".cbkt");

		std::ofstream out(outputPath, std::ios::binary);
		if (!out) {
			CBK_AC_ERROR("Failed to create output file: {}", outputPath.string());
			stbi_image_free(data);
			return;
		}

		out.write(reinterpret_cast<const char*>(&header), sizeof(header));
		out.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);

		stbi_image_free(data);
		CBK_AC_INFO("Converted: {} -> {}", path, outputPath.string());
	}

	void TextureConverter::packMetalRough(std::string_view metalPath, std::string_view roughPath,
	                                      std::string_view outputPath) {
		int mw, mh, mc;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* metalData = stbi_load(metalPath.data(), &mw, &mh, &mc, 1);
		if (!metalData) {
			CBK_AC_ERROR("Failed to load metalness texture: {}", metalPath);
			return;
		}

		int rw, rh, rc;
		stbi_uc* roughData = stbi_load(roughPath.data(), &rw, &rh, &rc, 1);
		if (!roughData) {
			CBK_AC_ERROR("Failed to load roughness texture: {}", roughPath);
			stbi_image_free(metalData);
			return;
		}

		int width = mw, height = mh;
		if (rw != mw || rh != mh) {
			CBK_AC_WARN("Metal ({}x{}) and Rough ({}x{}) dimensions differ, using metal size",
			             mw, mh, rw, rh);
		}

		uint32_t pixelCount = static_cast<uint32_t>(width) * height;
		uint32_t channels = 3;
		uint32_t dataSize = pixelCount * channels;
		std::vector<uint8_t> packed(dataSize);

		for (uint32_t i = 0; i < pixelCount; i++) {
			uint8_t metal = metalData[i];
			uint8_t rough = (i < static_cast<uint32_t>(rw * rh)) ? roughData[i] : 0;
			packed[i * 3 + 0] = 0;       // R = unused
			packed[i * 3 + 1] = rough;   // G = roughness
			packed[i * 3 + 2] = metal;   // B = metalness
		}

		stbi_image_free(metalData);
		stbi_image_free(roughData);

		std::vector<uint8_t> resized =
		    downscaleToFit(packed.data(), width, height, static_cast<int>(channels), s_MaxDimension);
		const uint8_t* pixels = resized.empty() ? packed.data() : resized.data();
		dataSize = static_cast<uint32_t>(width) * height * channels;

		int compressedCapacity = LZ4_compressBound(dataSize);
		std::vector<char> compressedData(compressedCapacity);
		int compressedSize = LZ4_compress_default(
			reinterpret_cast<const char*>(pixels), compressedData.data(),
			dataSize, compressedCapacity);

		if (compressedSize <= 0) {
			CBK_AC_ERROR("LZ4 compression failed for packed MetalRough");
			return;
		}

		TextureHeader header{
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.channels = channels,
			.compressedSize = static_cast<uint32_t>(compressedSize),
			.uncompressedSize = dataSize
		};

		std::ofstream out(outputPath.data(), std::ios::binary);
		if (!out) {
			CBK_AC_ERROR("Failed to create output file: {}", outputPath);
			return;
		}

		out.write(reinterpret_cast<const char*>(&header), sizeof(header));
		out.write(compressedData.data(), compressedSize);

		CBK_AC_INFO("Packed MetalRough: {} + {} -> {}", metalPath, roughPath, outputPath);
	}
} // namespace cbk::ac