#include "TextureConverter.h"

#include <algorithm>
#include <bit>
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
		// Box-filter downscale of an interleaved 8-bit image from (srcW, srcH) to
		// (dstW, dstH). Used both for the optional max-dimension cap and for the
		// per-level downsample when building the mip chain.
		std::vector<uint8_t> boxDownscale(const uint8_t* src, int srcW, int srcH,
		                                  int dstW, int dstH, int channels) {
			std::vector<uint8_t> dst(static_cast<size_t>(dstW) * dstH * channels);
			for (int y = 0; y < dstH; y++) {
				const int sy0 = y * srcH / dstH;
				const int sy1 = std::max(sy0 + 1, (y + 1) * srcH / dstH);
				for (int x = 0; x < dstW; x++) {
					const int sx0 = x * srcW / dstW;
					const int sx1 = std::max(sx0 + 1, (x + 1) * srcW / dstW);
					for (int c = 0; c < channels; c++) {
						uint32_t sum = 0, n = 0;
						for (int sy = sy0; sy < sy1; sy++)
							for (int sx = sx0; sx < sx1; sx++) {
								sum += src[(static_cast<size_t>(sy) * srcW + sx) * channels + c];
								n++;
							}
						dst[(static_cast<size_t>(y) * dstW + x) * channels + c] =
						    static_cast<uint8_t>(sum / n);
					}
				}
			}
			return dst;
		}

		// Cap to maxDim using the same box filter. Returns {} when no resize was
		// needed; otherwise width/height are updated in place.
		std::vector<uint8_t> downscaleToFit(const uint8_t* src, int& width, int& height,
		                                    int channels, uint32_t maxDim) {
			if (maxDim == 0 || (width <= static_cast<int>(maxDim) && height <= static_cast<int>(maxDim)))
				return {};

			const float scale = std::min(static_cast<float>(maxDim) / width,
			                             static_cast<float>(maxDim) / height);
			const int dstW = std::max(1, static_cast<int>(width * scale));
			const int dstH = std::max(1, static_cast<int>(height * scale));

			auto dst = boxDownscale(src, width, height, dstW, dstH, channels);
			width = dstW;
			height = dstH;
			return dst;
		}

		uint32_t mipLevelCountFor(uint32_t width, uint32_t height) {
			return static_cast<uint32_t>(std::bit_width(std::max(width, height)));
		}

		// Build the full mip chain from a level-0 image. The output buffer holds
		// levels sequentially with no padding; `offsets[i]` is the byte offset of
		// mip i (offsets.back() == total size). Each successive level is a 2x box
		// downsample of the previous level, with dimensions clamped to >= 1 so
		// non-power-of-two textures terminate at 1x1.
		std::vector<uint8_t> generateMipChain(const uint8_t* level0, int width, int height,
		                                      int channels, std::vector<uint32_t>& offsets) {
			const uint32_t levels = mipLevelCountFor(width, height);
			offsets.clear();
			offsets.reserve(levels + 1);

			// Pre-compute offsets so we can size the output buffer in one go.
			uint32_t totalBytes = 0;
			int w = width, h = height;
			for (uint32_t i = 0; i < levels; i++) {
				offsets.push_back(totalBytes);
				totalBytes += static_cast<uint32_t>(w) * h * channels;
				w = std::max(1, w / 2);
				h = std::max(1, h / 2);
			}
			offsets.push_back(totalBytes);

			std::vector<uint8_t> chain(totalBytes);
			const size_t level0Bytes = static_cast<size_t>(width) * height * channels;
			std::memcpy(chain.data(), level0, level0Bytes);

			int prevW = width, prevH = height;
			for (uint32_t i = 1; i < levels; i++) {
				const int curW = std::max(1, prevW / 2);
				const int curH = std::max(1, prevH / 2);
				auto downsampled = boxDownscale(chain.data() + offsets[i - 1], prevW, prevH,
				                                curW, curH, channels);
				std::memcpy(chain.data() + offsets[i], downsampled.data(), downsampled.size());
				prevW = curW;
				prevH = curH;
			}
			return chain;
		}

		bool writeCbkt(const std::filesystem::path& outputPath, int width, int height, int channels,
		               const uint8_t* level0Pixels) {
			std::vector<uint32_t> offsets;
			auto chain = generateMipChain(level0Pixels, width, height, channels, offsets);
			const uint32_t mipLevels = static_cast<uint32_t>(offsets.size() - 1);
			const uint32_t uncompressedSize = static_cast<uint32_t>(chain.size());

			int compressedCapacity = LZ4_compressBound(uncompressedSize);
			std::vector<char> compressedData(compressedCapacity);
			int compressedSize = LZ4_compress_default(reinterpret_cast<const char*>(chain.data()),
			                                          compressedData.data(),
			                                          uncompressedSize, compressedCapacity);
			if (compressedSize <= 0) {
				CBK_AC_ERROR("LZ4 compression failed for: {}", outputPath.string());
				return false;
			}

			cbk::common::TextureHeader header{
				.width = static_cast<uint32_t>(width),
				.height = static_cast<uint32_t>(height),
				.channels = static_cast<uint32_t>(channels),
				.mipLevels = mipLevels,
				.compressedSize = static_cast<uint32_t>(compressedSize),
				.uncompressedSize = uncompressedSize,
			};

			std::ofstream out(outputPath, std::ios::binary);
			if (!out) {
				CBK_AC_ERROR("Failed to create output file: {}", outputPath.string());
				return false;
			}
			out.write(reinterpret_cast<const char*>(&header), sizeof(header));
			out.write(compressedData.data(), compressedSize);
			return true;
		}
	} // namespace

	void TextureConverter::convert(std::string_view path) {
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = stbi_load(path.data(), &width, &height, &channels, 0);

		if (!data) {
			CBK_AC_ERROR("Failed to load texture: {}", path);
			return;
		}

		std::vector<uint8_t> resized = downscaleToFit(data, width, height, channels, s_MaxDimension);
		const uint8_t* pixels = resized.empty() ? data : resized.data();

		std::filesystem::path outputPath(path);
		outputPath.replace_extension(".cbkt");

		if (writeCbkt(outputPath, width, height, channels, pixels))
			CBK_AC_INFO("Converted: {} -> {} ({}x{}, {} channels, {} mips)", path, outputPath.string(),
			             width, height, channels, mipLevelCountFor(width, height));

		stbi_image_free(data);
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

		const uint32_t pixelCount = static_cast<uint32_t>(width) * height;
		const int channels = 3;
		std::vector<uint8_t> packed(static_cast<size_t>(pixelCount) * channels);

		for (uint32_t i = 0; i < pixelCount; i++) {
			uint8_t metal = metalData[i];
			uint8_t rough = (i < static_cast<uint32_t>(rw * rh)) ? roughData[i] : 0;
			packed[i * 3 + 0] = 0;       // R = unused
			packed[i * 3 + 1] = rough;   // G = roughness
			packed[i * 3 + 2] = metal;   // B = metalness
		}

		stbi_image_free(metalData);
		stbi_image_free(roughData);

		std::vector<uint8_t> resized = downscaleToFit(packed.data(), width, height, channels, s_MaxDimension);
		const uint8_t* pixels = resized.empty() ? packed.data() : resized.data();

		if (writeCbkt(outputPath, width, height, channels, pixels))
			CBK_AC_INFO("Packed MetalRough: {} + {} -> {} ({}x{}, {} mips)", metalPath, roughPath,
			             outputPath, width, height, mipLevelCountFor(width, height));
	}
} // namespace cbk::ac
