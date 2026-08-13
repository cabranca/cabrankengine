#include <pch.h>
#include "VulkanTexture.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <lz4.h>

#include "VkCheck.h"
#include "VulkanDeviceContext.h"
#include "VulkanRendererAPI.h"

namespace cbk::platform::vk {

	using namespace rendering;

	namespace {
		VkFormat toVkFormat(ImageFormat format) {
			switch (format) {
				case ImageFormat::R8:
					return VK_FORMAT_R8_UNORM;
				case ImageFormat::RGB8:
					return VK_FORMAT_R8G8B8_UNORM;
				case ImageFormat::RGBA8:
					return VK_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA32F:
					return VK_FORMAT_R32G32B32A32_SFLOAT;
				default:
					return VK_FORMAT_R8G8B8A8_UNORM;
			}
		}
	} // namespace

	VulkanTexture::VulkanTexture(const rendering::TextureSpecification& specification)
	    : m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height) {}

	VulkanTexture::VulkanTexture(const std::string& path, bool sRGB) : m_Path(path) {
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode))
			CBK_CORE_ERROR("Cannot find the path {0} - Error: {1}", path, errorCode.message());

		const auto size = std::filesystem::file_size(path, errorCode);
		if (errorCode)
			CBK_CORE_ERROR("Size check failure for file {0} - Error: {1}", path, errorCode.message());

		std::ifstream file(path, std::ios::binary);
		if (!file)
			CBK_CORE_ERROR("Cannot open file {0}", path);

		TextureHeader header;
		if (!file.read(reinterpret_cast<char*>(&header), sizeof(TextureHeader)))
			CBK_CORE_ERROR("Cannot read the file {0}", path);
		if (header.magic != 0x43424B54) // "CBKT"
			CBK_CORE_ERROR("Wrong extension of file {0} - .cbk expected!", path);
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

		// Build per-level offsets matching the converter's layout (tight packing,
		// dimensions clamped to >= 1).
		auto buildMipLevels = [&](uint32_t bytesPerPixel) {
			std::vector<MipLevel> mips;
			mips.reserve(mipLevels);
			uint32_t w = m_Width, h = m_Height;
			VkDeviceSize offset = 0;
			for (uint32_t i = 0; i < mipLevels; i++) {
				const VkDeviceSize size = static_cast<VkDeviceSize>(w) * h * bytesPerPixel;
				mips.push_back({ w, h, offset, size });
				offset += size;
				w = std::max(1u, w / 2);
				h = std::max(1u, h / 2);
			}
			return mips;
		};

		if (header.channels == 3) {
			// 24-bit RGB (VK_FORMAT_R8G8B8_UNORM) is not a required format for
			// optimal-tiled sampled images and is unsupported on most GPUs.
			// Expand each mip to RGBA8, which the Vulkan spec guarantees.
			auto srcMips = buildMipLevels(3);
			VkDeviceSize totalRgba = 0;
			for (const auto& m: srcMips)
				totalRgba += (m.byteSize / 3) * 4;
			std::vector<uint8_t> rgba(totalRgba);

			std::vector<MipLevel> dstMips;
			dstMips.reserve(mipLevels);
			VkDeviceSize dstOffset = 0;
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
			uploadPixels(sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM, rgba.data(), totalRgba, dstMips);
		} else {
			VkFormat format;
			uint32_t bpp;
			switch (header.channels) {
				case 1:
					format = VK_FORMAT_R8_UNORM;
					bpp = 1;
					break;
				case 2:
					format = VK_FORMAT_R8G8_UNORM;
					bpp = 2;
					break;
				case 4:
					format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
					bpp = 4;
					break;
				default:
					CBK_CORE_ERROR("VulkanTexture(): unsupported channel count {0} in {1}", header.channels, path);
					return;
			}
			auto mips = buildMipLevels(bpp);
			uploadPixels(format, uncompressedBuffer.data(), uncompressedBuffer.size(), mips);
		}
	}

	VulkanTexture::VulkanTexture(const FT_Face& face) {
		// FreeType renders each glyph as an 8-bit coverage bitmap; upload it as a
		// single-channel R8 texture. Empty glyphs (e.g. space) have a zero-size
		// bitmap — clamp to 1x1 so vkCreateImage gets a valid extent.
		const uint32_t glyphW = face->glyph->bitmap.width;
		const uint32_t glyphH = face->glyph->bitmap.rows;
		m_Width = glyphW > 0 ? glyphW : 1;
		m_Height = glyphH > 0 ? glyphH : 1;

		if (glyphW > 0 && glyphH > 0) {
			const VkDeviceSize size = static_cast<VkDeviceSize>(glyphW) * glyphH;
			std::vector<MipLevel> mips{ { glyphW, glyphH, 0, size } };
			uploadPixels(VK_FORMAT_R8_UNORM, face->glyph->bitmap.buffer, size, mips);
		} else {
			const uint8_t empty = 0;
			std::vector<MipLevel> mips{ { 1, 1, 0, 1 } };
			uploadPixels(VK_FORMAT_R8_UNORM, &empty, 1, mips);
		}
	}

	void VulkanTexture::uploadPixels(VkFormat format, const void* data, VkDeviceSize dataSize, const std::vector<MipLevel>& mips) {
		CBK_CORE_ASSERT(!mips.empty(), "VulkanTexture::uploadPixels(): mip list must not be empty");
		auto ctx = &VulkanRendererAPI::getContext();
		const uint32_t mipLevels = static_cast<uint32_t>(mips.size());

		VkImageCreateInfo texImgCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			                        .imageType = VK_IMAGE_TYPE_2D,
			                        .format = format,
			                        .extent = { .width = m_Width, .height = m_Height, .depth = 1 },
			                        .mipLevels = mipLevels,
			                        .arrayLayers = 1,
			                        .samples = VK_SAMPLE_COUNT_1_BIT,
			                        .tiling = VK_IMAGE_TILING_OPTIMAL,
			                        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
		VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
		VK_CHECK(vmaCreateImage(ctx->getAllocator(), &texImgCI, &texImageAllocCI, &m_Texture.image, &m_Texture.allocation, nullptr));
		VkImageViewCreateInfo texVewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			                            .image = m_Texture.image,
			                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
			                            .format = texImgCI.format,
			                            .subresourceRange = {
			                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = mipLevels, .layerCount = 1 } };
		VK_CHECK(vkCreateImageView(ctx->getDevice(), &texVewCI, nullptr, &m_Texture.view));

		// Upload
		VkBuffer imgSrcBuffer{};
		VmaAllocation imgSrcAllocation{};
		VkBufferCreateInfo imgSrcBufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			                               .size = dataSize,
			                               .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
		VmaAllocationCreateInfo imgSrcAllocCI{ .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			                                            VMA_ALLOCATION_CREATE_MAPPED_BIT,
			                                   .usage = VMA_MEMORY_USAGE_AUTO };
		VmaAllocationInfo imgSrcAllocInfo{};
		VK_CHECK(vmaCreateBuffer(ctx->getAllocator(), &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
		memcpy(imgSrcAllocInfo.pMappedData, data, dataSize);
		VkFenceCreateInfo fenceOneTimeCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkFence fenceOneTime{};
		VK_CHECK(vkCreateFence(ctx->getDevice(), &fenceOneTimeCI, nullptr, &fenceOneTime));
		VkCommandPool commandPool{ VK_NULL_HANDLE };
		VkCommandPoolCreateInfo commandPoolCI{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			                                   .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			                                   .queueFamilyIndex = ctx->getQueueFamily() };
		VK_CHECK(vkCreateCommandPool(ctx->getDevice(), &commandPoolCI, nullptr, &commandPool));
		VkCommandBuffer cbOneTime{};
		VkCommandBufferAllocateInfo cbOneTimeAI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			                                     .commandPool = commandPool,
			                                     .commandBufferCount = 1 };
		VK_CHECK(vkAllocateCommandBuffers(ctx->getDevice(), &cbOneTimeAI, &cbOneTime));
		VkCommandBufferBeginInfo cbOneTimeBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			                                  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		VK_CHECK(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));
		VkImageMemoryBarrier2 barrierTexImage{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                   .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			                                   .srcAccessMask = VK_ACCESS_2_NONE,
			                                   .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			                                   .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			                                   .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                                   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                                   .image = m_Texture.image,
			                                   .subresourceRange = {
			                                       .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = mipLevels, .layerCount = 1 } };
		VkDependencyInfo barrierTexInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                             .imageMemoryBarrierCount = 1,
			                             .pImageMemoryBarriers = &barrierTexImage };
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);

		std::vector<VkBufferImageCopy> copyRegions;
		copyRegions.reserve(mipLevels);
		for (uint32_t level = 0; level < mipLevels; level++) {
			copyRegions.push_back(
			    VkBufferImageCopy{ .bufferOffset = mips[level].byteOffset,
			                       .imageSubresource{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = level, .layerCount = 1 },
			                       .imageExtent{ .width = mips[level].width, .height = mips[level].height, .depth = 1 } });
		}
		vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, m_Texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                       static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

		VkImageMemoryBarrier2 barrierTexRead{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                  .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			                                  .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			                                  .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			                                  .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			                                  .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                                  .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			                                  .image = m_Texture.image,
			                                  .subresourceRange = {
			                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = mipLevels, .layerCount = 1 } };
		barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		VK_CHECK(vkEndCommandBuffer(cbOneTime));
		VkSubmitInfo oneTimeSI{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cbOneTime };
		VK_CHECK(vkQueueSubmit(ctx->getQueue().getQueue(), 1, &oneTimeSI, fenceOneTime));
		VK_CHECK(vkWaitForFences(ctx->getDevice(), 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
		vkDestroyFence(ctx->getDevice(), fenceOneTime, nullptr);
		vmaDestroyBuffer(ctx->getAllocator(), imgSrcBuffer, imgSrcAllocation);
		vkDestroyCommandPool(ctx->getDevice(), commandPool, nullptr);

		// Sampler — trilinear filtering across the full mip chain plus anisotropic
		// filtering. VK_LOD_CLAMP_NONE lets the implementation pick any mip in the
		// chain, which is what we want now that we ship the full pyramid.
		// Anisotropy must be clamped to the physical device's limit or
		// vkCreateSampler returns VK_ERROR_VALIDATION_FAILED_EXT.
		VkPhysicalDeviceProperties devProps{};
		vkGetPhysicalDeviceProperties(ctx->getPhysicalDevice(), &devProps);
		VkSamplerCreateInfo samplerCI{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = std::min(16.0f, devProps.limits.maxSamplerAnisotropy),
			.maxLod = VK_LOD_CLAMP_NONE,
		};
		VK_CHECK(vkCreateSampler(ctx->getDevice(), &samplerCI, nullptr, &m_Texture.sampler));
		m_TexDescriptorInfo = VkDescriptorImageInfo{ .sampler = m_Texture.sampler,
			                                         .imageView = m_Texture.view,
			                                         .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL };

		m_IsLoaded = true;
	}

	VulkanTexture::~VulkanTexture() {
		auto ctx = &VulkanRendererAPI::getContext();
		if (m_Texture.view != VK_NULL_HANDLE)
			vkDestroyImageView(ctx->getDevice(), m_Texture.view, nullptr);
		if (m_Texture.sampler != VK_NULL_HANDLE)
			vkDestroySampler(ctx->getDevice(), m_Texture.sampler, nullptr);
		if (m_Texture.image != VK_NULL_HANDLE)
			vmaDestroyImage(ctx->getAllocator(), m_Texture.image, m_Texture.allocation);
	}

	const rendering::TextureSpecification& VulkanTexture::getSpecification() const {
		return m_Specification;
	}

	uint32_t VulkanTexture::getWidth() const {
		return m_Width;
	}

	uint32_t VulkanTexture::getHeight() const {
		return m_Height;
	}

	uint64_t VulkanTexture::getRendererID() const {
		return reinterpret_cast<uint64_t>(m_Texture.image);
	}

	const std::string& VulkanTexture::getPath() const {
		return m_Path;
	}

	// Uploads raw pixels into a spec-constructed texture (e.g. the 1x1 white fallback).
	void VulkanTexture::setData(void* data, uint32_t size) {
		std::vector<MipLevel> mips{ { m_Width, m_Height, 0, size } };
		uploadPixels(toVkFormat(m_Specification.Format), data, size, mips);
	}

	// Binds the texture to a specific slot in the graphics API, allowing it to be used in rendering.
	void VulkanTexture::bind(uint32_t slot) const {}

	// Returns whether the texture is loaded successfully.
	bool VulkanTexture::isLoaded() const {
		return m_IsLoaded;
	}

	// Returns whether both textures are equal based on their specifications and renderer IDs.
	bool VulkanTexture::operator==(const Texture& other) const {
		return getRendererID() == other.getRendererID();
	}

	const VkDescriptorImageInfo* VulkanTexture::getDescriptor() const {
		return &m_TexDescriptorInfo;
	}
} // namespace cbk::platform::vk
