#include <pch.h>
#include "VulkanTexture.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <lz4.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "VulkanDeviceContext.h"

namespace cbk::platform::vk {

	using namespace rendering;

	namespace {
		VkFormat toVkFormat(ImageFormat format) {
			switch (format) {
				case ImageFormat::R8:      return VK_FORMAT_R8_UNORM;
				case ImageFormat::RGB8:    return VK_FORMAT_R8G8B8_UNORM;
				case ImageFormat::RGBA8:   return VK_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
				default:                   return VK_FORMAT_R8G8B8A8_UNORM;
			}
		}
	} // namespace

	VulkanTexture::VulkanTexture(const rendering::TextureSpecification& specification)
	    : m_Specification(specification), m_Width(m_Specification.Width), m_Height(m_Specification.Height) {}

	VulkanTexture::VulkanTexture(const std::string& path) : m_Path(path) {
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

			VkFormat format;
			switch (header.channels) {
				case 1:
					format = VK_FORMAT_R8_UNORM;
					break;
				case 2:
					format = VK_FORMAT_R8G8_UNORM;
					break;
				case 3:
					format = VK_FORMAT_R8G8B8_UNORM;
					break;
				case 4:
					format = VK_FORMAT_R8G8B8A8_UNORM;
					break;
			}

			uploadPixels(format, uncompressedBuffer.data(), uncompressedBuffer.size());
		}
	}

	VulkanTexture::VulkanTexture(const FT_Face& face) {
		// FreeType renders each glyph as an 8-bit coverage bitmap; upload it as a
		// single-channel R8 texture. Empty glyphs (e.g. space) have a zero-size
		// bitmap — clamp to 1x1 so vkCreateImage gets a valid extent.
		const uint32_t glyphW = face->glyph->bitmap.width;
		const uint32_t glyphH = face->glyph->bitmap.rows;
		m_Width  = glyphW > 0 ? glyphW : 1;
		m_Height = glyphH > 0 ? glyphH : 1;

		if (glyphW > 0 && glyphH > 0) {
			uploadPixels(VK_FORMAT_R8_UNORM, face->glyph->bitmap.buffer,
			             static_cast<VkDeviceSize>(glyphW) * glyphH);
		} else {
			const uint8_t empty = 0;
			uploadPixels(VK_FORMAT_R8_UNORM, &empty, 1);
		}
	}

	void VulkanTexture::uploadPixels(VkFormat format, const void* data, VkDeviceSize dataSize) {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());

		VkImageCreateInfo texImgCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			                        .imageType = VK_IMAGE_TYPE_2D,
			                        .format = format,
			                        .extent = { .width = m_Width, .height = m_Height, .depth = 1 },
			                        .mipLevels = 1,
			                        .arrayLayers = 1,
			                        .samples = VK_SAMPLE_COUNT_1_BIT,
			                        .tiling = VK_IMAGE_TILING_OPTIMAL,
			                        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
		VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
		auto vkResult = vmaCreateImage(ctx->getAllocator(), &texImgCI, &texImageAllocCI, &m_Texture.image, &m_Texture.allocation, nullptr);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error creating image", static_cast<int>(vkResult));
			return;
		}
		VkImageViewCreateInfo texVewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = m_Texture.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = texImgCI.format,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		vkResult = vkCreateImageView(ctx->getLogicalDevice(), &texVewCI, nullptr, &m_Texture.view);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error creating image view", static_cast<int>(vkResult));
			return;
		}

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
		vkResult =
		    vmaCreateBuffer(ctx->getAllocator(), &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error creating texture buffer", static_cast<int>(vkResult));
			return;
		}
		memcpy(imgSrcAllocInfo.pMappedData, data, dataSize);
		VkFenceCreateInfo fenceOneTimeCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkFence fenceOneTime{};
		vkResult = vkCreateFence(ctx->getLogicalDevice(), &fenceOneTimeCI, nullptr, &fenceOneTime);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error creating fence", static_cast<int>(vkResult));
			return;
		}
		VkCommandPool commandPool{ VK_NULL_HANDLE };
		VkCommandPoolCreateInfo commandPoolCI{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, .queueFamilyIndex = ctx->getQueueFamily() };
		vkResult = vkCreateCommandPool(ctx->getLogicalDevice(), &commandPoolCI, nullptr, &commandPool);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error creating command pool", static_cast<int>(vkResult));
			return;
		}
		VkCommandBuffer cbOneTime{};
		VkCommandBufferAllocateInfo cbOneTimeAI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			                                     .commandPool = commandPool,
			                                     .commandBufferCount = 1 };
		vkResult = vkAllocateCommandBuffers(ctx->getLogicalDevice(), &cbOneTimeAI, &cbOneTime);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error allocating command buffer", static_cast<int>(vkResult));
			return;
		}
		VkCommandBufferBeginInfo cbOneTimeBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			                                  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		vkResult = vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error beginning command buffer", static_cast<int>(vkResult));
			return;
		}
		VkImageMemoryBarrier2 barrierTexImage{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                   .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			                                   .srcAccessMask = VK_ACCESS_2_NONE,
			                                   .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			                                   .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			                                   .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                                   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                                   .image = m_Texture.image,
			                                   .subresourceRange = {
			                                       .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		VkDependencyInfo barrierTexInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                             .imageMemoryBarrierCount = 1,
			                             .pImageMemoryBarriers = &barrierTexImage };
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		VkBufferImageCopy copyRegion{ .bufferOffset = 0,
			                          .imageSubresource{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .layerCount = 1 },
			                          .imageExtent{ .width = m_Width, .height = m_Height, .depth = 1 } };
		vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, m_Texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		VkImageMemoryBarrier2 barrierTexRead{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                  .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			                                  .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			                                  .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			                                  .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			                                  .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                                  .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			                                  .image = m_Texture.image,
			                                  .subresourceRange = {
			                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		vkResult = vkEndCommandBuffer(cbOneTime);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error ending command buffer", static_cast<int>(vkResult));
			return;
		}
		VkSubmitInfo oneTimeSI{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cbOneTime };
		vkResult = vkQueueSubmit(ctx->getDeviceQueue(), 1, &oneTimeSI, fenceOneTime);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error submitting queue", static_cast<int>(vkResult));
			return;
		}
		vkResult = vkWaitForFences(ctx->getLogicalDevice(), 1, &fenceOneTime, VK_TRUE, UINT64_MAX);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error waiting for fence", static_cast<int>(vkResult));
			return;
		}
		vkDestroyFence(ctx->getLogicalDevice(), fenceOneTime, nullptr);
		vmaDestroyBuffer(ctx->getAllocator(), imgSrcBuffer, imgSrcAllocation);
		vkDestroyCommandPool(ctx->getLogicalDevice(), commandPool, nullptr);

		// Sampler
		VkSamplerCreateInfo samplerCI{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 8.0f,
			.maxLod = 1.f,
		};
		vkResult = vkCreateSampler(ctx->getLogicalDevice(), &samplerCI, nullptr, &m_Texture.sampler);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture(): error creating sampler", static_cast<int>(vkResult));
			return;
		}
		m_TexDescriptorInfo = VkDescriptorImageInfo{ .sampler = m_Texture.sampler,
			                                         .imageView = m_Texture.view,
			                                         .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL };

		m_IsLoaded = true;
	}

	VulkanTexture::~VulkanTexture() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		vkDestroyImageView(ctx->getLogicalDevice(), m_Texture.view, nullptr);
		vkDestroySampler(ctx->getLogicalDevice(), m_Texture.sampler, nullptr);
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
		uploadPixels(toVkFormat(m_Specification.Format), data, size);
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
