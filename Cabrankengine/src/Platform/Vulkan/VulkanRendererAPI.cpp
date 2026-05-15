#include <pch.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include "VulkanRendererAPI.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/VertexArray.h>
#include <Cabrankengine/Renderer/Buffer.h>

namespace {
	struct VkCtx {
		VkInstance instance{ VK_NULL_HANDLE };
		std::vector<VkPhysicalDevice> devices;
		uint32_t deviceIdx;
		VkDevice device{ VK_NULL_HANDLE };
		uint32_t queueFamily{ 0 };
		VkQueue queue{ VK_NULL_HANDLE };
		VkSurfaceKHR surface{ VK_NULL_HANDLE };
		VkSurfaceCapabilitiesKHR surfaceCaps{};
		VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
		VkCommandPool commandPool{ VK_NULL_HANDLE };
		VkPipeline pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
		VkImageCreateInfo depthImageCI{};
		VkImage depthImage{ VK_NULL_HANDLE };
		VmaAllocator allocator{ VK_NULL_HANDLE };
		VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
		VmaAllocation depthImageAllocation{ VK_NULL_HANDLE };
		VkImageView depthImageView{ VK_NULL_HANDLE };
		VkSwapchainCreateInfoKHR swapchainCI{};
		uint32_t imageCount{ 0 };
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		std::array<VkCommandBuffer, 2> commandBuffers{};
		std::array<VkFence, 2> fences{};
		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		std::array<VkSemaphore, 2> imageAcquiredSemaphores{};
		std::vector<VkSemaphore> renderCompleteSemaphores;
		VmaAllocation vBufferAllocation{ VK_NULL_HANDLE };
		VkBuffer vBuffer{ VK_NULL_HANDLE };
		VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSetTex{ VK_NULL_HANDLE };
		Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
		static constexpr VkFormat imageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
	} ctx;
} // namespace

namespace cbk::platform::vk {

	using namespace math;
	using namespace rendering;

	void VulkanRendererAPI::init() {
		if (glfwVulkanSupported() != GLFW_TRUE) {
			CBK_CORE_ERROR("Vulkan not supported");
			return;
		}

		auto vkResult = volkInitialize();
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI(): error initializing volk ({})", static_cast<int>(vkResult));
			return;
		}

		if (!createVulkanInstance())
			return;

		if (!createVulkanDevice())
			return;

		if (!createAllocator())
			return;

		if (!createSwapchain())
			return;

		if (!getSwapchainImages())
			return;

		if (!createDepthAttachment())
			return;

		// // Mesh data
		// tinyobj::attrib_t attrib;
		// std::vector<tinyobj::shape_t> shapes;
		// std::vector<tinyobj::material_t> materials;
		// chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/suzanne.obj"));
		// const VkDeviceSize indexCount{ shapes[0].mesh.indices.size() };
		// std::vector<Vertex> vertices{};
		// std::vector<uint16_t> indices{};
		// // Load vertex and index data
		// for (auto& index : shapes[0].mesh.indices) {
		// 	Vertex v{
		// 		.pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1],
		// attrib.vertices[index.vertex_index * 3 + 2] }, 		.normal = { attrib.normals[index.normal_index * 3],
		// -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] }, 		.uv = {
		// attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
		// 	};
		// 	vertices.push_back(v);
		// 	indices.push_back(indices.size());
		// }
		// VkDeviceSize vBufSize{ sizeof(Vertex) * vertices.size() };
		// VkDeviceSize iBufSize{ sizeof(uint16_t) * indices.size() };
		// VkBufferCreateInfo bufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = vBufSize + iBufSize, .usage =
		// VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT }; VmaAllocationCreateInfo vBufferAllocCI{ .flags =
		// VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
		// VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = VMA_MEMORY_USAGE_AUTO }; VmaAllocationInfo vBufferAllocInfo{};
		// chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));
		// memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
		// memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);
		// // Shader data buffers
		// for (auto i = 0; i < maxFramesInFlight; i++) {
		// 	VkBufferCreateInfo uBufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = sizeof(ShaderData), .usage =
		// VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT }; 	VmaAllocationCreateInfo uBufferAllocCI{ .flags =
		// VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
		// VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = VMA_MEMORY_USAGE_AUTO }; 	chk(vmaCreateBuffer(allocator, &uBufferCI, &uBufferAllocCI,
		// &shaderDataBuffers[i].buffer, &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo)); VkBufferDeviceAddressInfo
		// uBufferBdaInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = shaderDataBuffers[i].buffer };
		// 	shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
		// }

		if (!createSyncObjects())
			return;

		if (!createCommandPool())
			return;

		// // Texture images
		// std::vector<VkDescriptorImageInfo> textureDescriptors{};
		// for (auto i = 0; i < textures.size(); i++) {
		// 	ktxTexture* ktxTexture{ nullptr };
		// 	std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
		// 	ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		// 	VkImageCreateInfo texImgCI{
		// 		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		// 		.imageType = VK_IMAGE_TYPE_2D,
		// 		.format = ktxTexture_GetVkFormat(ktxTexture),
		// 		.extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1 },
		// 		.mipLevels = ktxTexture->numLevels,
		// 		.arrayLayers = 1,
		// 		.samples = VK_SAMPLE_COUNT_1_BIT,
		// 		.tiling = VK_IMAGE_TILING_OPTIMAL,
		// 		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		// 		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		// 	};
		// 	VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
		// 	chk(vmaCreateImage(allocator, &texImgCI, &texImageAllocCI, &textures[i].image, &textures[i].allocation, nullptr));
		// 	VkImageViewCreateInfo texVewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = textures[i].image, .viewType =
		// VK_IMAGE_VIEW_TYPE_2D, .format = texImgCI.format, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount =
		// ktxTexture->numLevels, .layerCount = 1 } }; 	chk(vkCreateImageView(device, &texVewCI, nullptr, &textures[i].view));
		// 	// Upload
		// 	VkBuffer imgSrcBuffer{};
		// 	VmaAllocation imgSrcAllocation{};
		// 	VkBufferCreateInfo imgSrcBufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = (uint32_t)ktxTexture->dataSize, .usage
		// = VK_BUFFER_USAGE_TRANSFER_SRC_BIT }; 	VmaAllocationCreateInfo imgSrcAllocCI{ .flags =
		// VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = VMA_MEMORY_USAGE_AUTO };
		// 	VmaAllocationInfo imgSrcAllocInfo{};
		// 	chk(vmaCreateBuffer(allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
		// 	memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);
		// 	VkFenceCreateInfo fenceOneTimeCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		// 	VkFence fenceOneTime{};
		// 	chk(vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime));
		// 	VkCommandBuffer cbOneTime{};
		// 	VkCommandBufferAllocateInfo cbOneTimeAI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = commandPool,
		// .commandBufferCount = 1 }; 	chk(vkAllocateCommandBuffers(device, &cbOneTimeAI, &cbOneTime)); 	VkCommandBufferBeginInfo
		// cbOneTimeBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		// 	chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));
		// 	VkImageMemoryBarrier2 barrierTexImage{
		// 		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		// 		.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
		// 		.srcAccessMask = VK_ACCESS_2_NONE,
		// 		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		// 		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		// 		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		// 		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		// 		.image = textures[i].image,
		// 		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		// 	};
		// 	VkDependencyInfo barrierTexInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers
		// = &barrierTexImage }; 	vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo); 	std::vector<VkBufferImageCopy> copyRegions{}; 	for
		// (auto j = 0; j < ktxTexture->numLevels; j++) { 		ktx_size_t mipOffset{0}; 		KTX_error_code ret =
		// ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset); 		copyRegions.push_back({ 			.bufferOffset = mipOffset,
		// .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
		// .imageExtent{.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
		// 		});
		// 	}
		// 	vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		// static_cast<uint32_t>(copyRegions.size()), copyRegions.data()); 	VkImageMemoryBarrier2 barrierTexRead{ 		.sType =
		// VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, 		.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT, 		.srcAccessMask =
		// VK_ACCESS_TRANSFER_WRITE_BIT, 		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 		.dstAccessMask =
		// VK_ACCESS_SHADER_READ_BIT, 		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 		.newLayout =
		// VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, 		.image = textures[i].image, 		.subresourceRange = {.aspectMask =
		// VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		// 	};
		// 	barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
		// 	vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		// 	chk(vkEndCommandBuffer(cbOneTime));
		// 	VkSubmitInfo oneTimeSI{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cbOneTime };
		// 	chk(vkQueueSubmit(queue, 1, &oneTimeSI, fenceOneTime));
		// 	chk(vkWaitForFences(device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
		// 	vkDestroyFence(device, fenceOneTime, nullptr);
		// 	vmaDestroyBuffer(allocator, imgSrcBuffer, imgSrcAllocation);
		// 	// Sampler
		// 	VkSamplerCreateInfo samplerCI{
		// 		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		// 		.magFilter = VK_FILTER_LINEAR,
		// 		.minFilter = VK_FILTER_LINEAR,
		// 		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		// 		.anisotropyEnable = VK_TRUE,
		// 		.maxAnisotropy = 8.0f,
		// 		.maxLod = (float)ktxTexture->numLevels,
		// 	};
		// 	chk(vkCreateSampler(device, &samplerCI, nullptr, &textures[i].sampler));
		// 	ktxTexture_Destroy(ktxTexture);
		// 	textureDescriptors.push_back({ .sampler = textures[i].sampler, .imageView = textures[i].view, .imageLayout =
		// VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL });
		// }
		// // Descriptor (indexing)
		// VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
		// VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{ .sType =
		// VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, .bindingCount = 1, .pBindingFlags = &descVariableFlag };
		// VkDescriptorSetLayoutBinding descLayoutBindingTex{ .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount
		// = static_cast<uint32_t>(textures.size()), .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT }; VkDescriptorSetLayoutCreateInfo
		// descLayoutTexCI{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = &descBindingFlags, .bindingCount = 1,
		// .pBindings = &descLayoutBindingTex }; chk(vkCreateDescriptorSetLayout(device, &descLayoutTexCI, nullptr,
		// &descriptorSetLayoutTex)); VkDescriptorPoolSize poolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount =
		// static_cast<uint32_t>(textures.size()) }; VkDescriptorPoolCreateInfo descPoolCI{ .sType =
		// VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &poolSize };
		// chk(vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool));
		// uint32_t variableDescCount{ static_cast<uint32_t>(textures.size()) };
		// VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{ .sType =
		// VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT, .descriptorSetCount = 1, .pDescriptorCounts =
		// &variableDescCount}; VkDescriptorSetAllocateInfo texDescSetAlloc{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext
		// = &variableDescCountAI, .descriptorPool = descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &descriptorSetLayoutTex };
		// chk(vkAllocateDescriptorSets(device, &texDescSetAlloc, &descriptorSetTex));
		// VkWriteDescriptorSet writeDescSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = descriptorSetTex, .dstBinding = 0,
		// .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()), .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		// .pImageInfo = textureDescriptors.data() }; vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
		// // Initialize Slang shader compiler
		// slang::createGlobalSession(slangGlobalSession.writeRef());
		// auto slangTargets{ std::to_array<slang::TargetDesc>({ {.format{SLANG_SPIRV},
		// .profile{slangGlobalSession->findProfile("spirv_1_4")} } }) }; auto slangOptions{ std::to_array<slang::CompilerOptionEntry>({ {
		// slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1} } }) }; slang::SessionDesc
		// slangSessionDesc{ .targets{slangTargets.data()}, .targetCount{SlangInt(slangTargets.size())}, .defaultMatrixLayoutMode =
		// SLANG_MATRIX_LAYOUT_COLUMN_MAJOR, .compilerOptionEntries{slangOptions.data()},
		// .compilerOptionEntryCount{uint32_t(slangOptions.size())} };
		// // Load shader
		// Slang::ComPtr<slang::ISession> slangSession;
		// slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
		// Slang::ComPtr<slang::IModule> slangModule{ slangSession->loadModuleFromSource("triangle", "assets/shader.slang", nullptr,
		// nullptr) }; Slang::ComPtr<ISlangBlob> spirv; slangModule->getTargetCode(0, spirv.writeRef()); VkShaderModuleCreateInfo
		// shaderModuleCI{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = spirv->getBufferSize(), .pCode =
		// (uint32_t*)spirv->getBufferPointer() }; VkShaderModule shaderModule{}; chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr,
		// &shaderModule));
		// // Pipeline
		// VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(VkDeviceAddress) };
		// VkPipelineLayoutCreateInfo pipelineLayoutCI{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1,
		// .pSetLayouts = &descriptorSetLayoutTex, .pushConstantRangeCount = 1, .pPushConstantRanges = &pushConstantRange };
		// chk(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
		// std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
		// 	{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = shaderModule,
		// .pName = "main"}, 	{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		// .module = shaderModule, .pName = "main" }
		// };
		// VkVertexInputBindingDescription vertexBinding{ .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		// }; std::vector<VkVertexInputAttributeDescription> vertexAttributes{ 	{ .location = 0, .binding = 0, .format =
		// VK_FORMAT_R32G32B32_SFLOAT }, 	{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex,
		// normal) }, 	{ .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
		// };
		// VkPipelineVertexInputStateCreateInfo vertexInputState{
		// 	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		// 	.vertexBindingDescriptionCount = 1,
		// 	.pVertexBindingDescriptions = &vertexBinding,
		// 	.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
		// 	.pVertexAttributeDescriptions = vertexAttributes.data(),
		// };
		// VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		// .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST }; std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT,
		// VK_DYNAMIC_STATE_SCISSOR }; VkPipelineDynamicStateCreateInfo dynamicState{ .sType =
		// VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = 2, .pDynamicStates = dynamicStates.data() };
		// VkPipelineViewportStateCreateInfo viewportState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount =
		// 1, .scissorCount = 1 }; VkPipelineRasterizationStateCreateInfo rasterizationState{ .sType =
		// VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .lineWidth = 1.0f }; VkPipelineMultisampleStateCreateInfo
		// multisampleState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples =
		// VK_SAMPLE_COUNT_1_BIT }; VkPipelineDepthStencilStateCreateInfo depthStencilState{ .sType =
		// VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, .depthTestEnable = VK_TRUE, .depthWriteEnable = VK_TRUE,
		// .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL }; VkPipelineColorBlendAttachmentState blendAttachment{ .colorWriteMask = 0xF };
		// VkPipelineColorBlendStateCreateInfo colorBlendState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		// .attachmentCount = 1, .pAttachments = &blendAttachment }; VkPipelineRenderingCreateInfo renderingCI{ .sType =
		// VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, .colorAttachmentCount = 1, .pColorAttachmentFormats = &imageFormat,
		// .depthAttachmentFormat = ctx.depthFormat }; VkGraphicsPipelineCreateInfo pipelineCI{ 	.sType =
		// VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, 	.pNext = &renderingCI, 	.stageCount = 2, 	.pStages = shaderStages.data(),
		// 	.pVertexInputState = &vertexInputState,
		// 	.pInputAssemblyState = &inputAssemblyState,
		// 	.pViewportState = &viewportState,
		// 	.pRasterizationState = &rasterizationState,
		// 	.pMultisampleState = &multisampleState,
		// 	.pDepthStencilState = &depthStencilState,
		// 	.pColorBlendState = &colorBlendState,
		// 	.pDynamicState = &dynamicState,
		// 	.layout = pipelineLayout
		// };
		// chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

		// }
		// // Tear down
		// chk(vkDeviceWaitIdle(device));
		// for (auto i = 0; i < maxFramesInFlight; i++) {
		// 	vkDestroyFence(device, fences[i], nullptr);
		// 	vkDestroySemaphore(device, imageAcquiredSemaphores[i], nullptr);
		// 	vmaDestroyBuffer(allocator, shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
		// }
		// for (auto i = 0; i < renderCompleteSemaphores.size(); i++) {
		// 	vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
		// }
		// vmaDestroyImage(allocator, depthImage, depthImageAllocation);
		// vkDestroyImageView(device, depthImageView, nullptr);
		// for (auto i = 0; i < swapchainImageViews.size(); i++) {
		// 	vkDestroyImageView(device, swapchainImageViews[i], nullptr);
		// }
		// vmaDestroyBuffer(allocator, vBuffer, vBufferAllocation);
		// for (auto i = 0; i < textures.size(); i++) {
		// 	vkDestroyImageView(device, textures[i].view, nullptr);
		// 	vkDestroySampler(device, textures[i].sampler, nullptr);
		// 	vmaDestroyImage(allocator, textures[i].image, textures[i].allocation);
		// }
		// vkDestroyDescriptorSetLayout(device, descriptorSetLayoutTex, nullptr);
		// vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		// vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		// vkDestroyPipeline(device, pipeline, nullptr);
		// vkDestroySwapchainKHR(device, swapchain, nullptr);
		// vkDestroySurfaceKHR(instance, surface, nullptr);
		// vkDestroyCommandPool(device, commandPool, nullptr);
		// vkDestroyShaderModule(device, shaderModule, nullptr);
		// vmaDestroyAllocator(allocator);
		// SDL_DestroyWindow(window);
		// SDL_QuitSubSystem(SDL_INIT_VIDEO);
		// SDL_Quit();
		// vkDestroyDevice(device, nullptr);
		// vkDestroyInstance(instance, nullptr);
	}

	void VulkanRendererAPI::setClearColor(const Vector4& color) {}

	void VulkanRendererAPI::clear() {}

	void VulkanRendererAPI::draw(const Ref<VertexArray>& vertexArray) {
		if (!syncAndAcquire())
			return;

		// // UPDATE SHADER DATA -> MAYBE DO IT IN Renderer::submit()?
		// shaderData.projection = glm::perspective(glm::radians(45.0f), (float)windowSize.x / (float)windowSize.y, 0.1f, 32.0f);
		// shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
		// for (auto i = 0; i < 3; i++) {
		// 	auto instancePos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
		// 	shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
		// }
		// memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));

		if (!commitRenderCommands(vertexArray))
			return;

		if (!submitQueue())
			return;

		if (m_UpdateSwapchain)
			updateSwapchain();
	}

	void VulkanRendererAPI::drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) {}

	void VulkanRendererAPI::endFrame() {}

	void VulkanRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

	bool VulkanRendererAPI::createVulkanInstance() {
		VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			                       .pApplicationName = "Cabrankengine",
			                       .apiVersion = VK_API_VERSION_1_3 };
		uint32_t instanceExtensionsCount = 0;
		char const* const* instanceExtensions{ glfwGetRequiredInstanceExtensions(&instanceExtensionsCount) };
		VkInstanceCreateInfo instanceCI{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = instanceExtensionsCount,
			.ppEnabledExtensionNames = instanceExtensions,
		};
		auto vkResult = vkCreateInstance(&instanceCI, nullptr, &ctx.instance);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("vkCreateInstance: error creating instance ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::createVulkanDevice() {
		uint32_t deviceCount{ 0 };
		auto vkResult = vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("vkEnumeratePhysicalDevices: error enumerating physical devices ({})", static_cast<int>(vkResult));
			return false;
		}
		ctx.devices.resize(deviceCount);
		vkResult = vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, ctx.devices.data());
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("vkEnumeratePhysicalDevices: error enumerating physical devices ({})", static_cast<int>(vkResult));
			return false;
		}
		VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		vkGetPhysicalDeviceProperties2(ctx.devices[ctx.deviceIdx], &deviceProperties);
		CBK_CORE_INFO("Selected device: {}", deviceProperties.properties.deviceName);
		uint32_t queueFamilyCount{ 0 };
		vkGetPhysicalDeviceQueueFamilyProperties(ctx.devices[ctx.deviceIdx], &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(ctx.devices[ctx.deviceIdx], &queueFamilyCount, queueFamilies.data());
		for (size_t i = 0; i < queueFamilies.size(); i++) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				ctx.queueFamily = i;
				break;
			}
		}
		if (glfwGetPhysicalDevicePresentationSupport(ctx.instance, ctx.devices[ctx.deviceIdx], ctx.queueFamily) != GLFW_TRUE) {
			CBK_CORE_ERROR("VulkanRendererAPI: Selected queue family cannot present images");
			return false;
		}
		const float qfPriorities{ 1.f };
		VkDeviceQueueCreateInfo queueCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			                             .queueFamilyIndex = ctx.queueFamily,
			                             .queueCount = 1,
			                             .pQueuePriorities = &qfPriorities };
		VkPhysicalDeviceVulkan12Features enabledVk12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			                                                  .descriptorIndexing = true,
			                                                  .shaderSampledImageArrayNonUniformIndexing = true,
			                                                  .descriptorBindingVariableDescriptorCount = true,
			                                                  .runtimeDescriptorArray = true,
			                                                  .bufferDeviceAddress = true };
		VkPhysicalDeviceVulkan13Features enabledVk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			                                                  .pNext = &enabledVk12Features,
			                                                  .synchronization2 = true,
			                                                  .dynamicRendering = true };
		VkPhysicalDeviceFeatures enabledVk10Features{ .samplerAnisotropy = VK_TRUE };
		const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			                         .pNext = &enabledVk13Features,
			                         .queueCreateInfoCount = 1,
			                         .pQueueCreateInfos = &queueCI,
			                         .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			                         .ppEnabledExtensionNames = deviceExtensions.data(),
			                         .pEnabledFeatures = &enabledVk10Features };
		vkResult = vkCreateDevice(ctx.devices[ctx.deviceIdx], &deviceCI, nullptr, &ctx.device);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating Vulkan device ({})", static_cast<int>(vkResult));
			return false;
		}
		vkGetDeviceQueue(ctx.device, ctx.queueFamily, 0, &ctx.queue); // Why not check this?
		return true;
	}

	bool VulkanRendererAPI::createAllocator() {
		VmaVulkanFunctions vkFunctions{ .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
			                            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
			                            .vkCreateImage = vkCreateImage };
		VmaAllocatorCreateInfo allocatorCI{ .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			                                .physicalDevice = ctx.devices[ctx.deviceIdx],
			                                .device = ctx.device,
			                                .pVulkanFunctions = &vkFunctions,
			                                .instance = ctx.instance };
		auto vkResult = vmaCreateAllocator(&allocatorCI, &ctx.allocator);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating VMA Allocator ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::createSwapchain() {
		auto& window = cbk::Application::get().getWindow();
		auto glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());
		glfwCreateWindowSurface(ctx.instance, glfwWindow, nullptr, &ctx.surface);
		auto vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.devices[ctx.deviceIdx], ctx.surface, &ctx.surfaceCaps);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error getting surface capabilities ({})", static_cast<int>(vkResult));
			return false;
		}
		VkExtent2D swapchainExtent{ ctx.surfaceCaps.currentExtent };
		if (ctx.surfaceCaps.currentExtent.width == 0xFFFFFFFF) { // For wayland
			swapchainExtent = { .width = static_cast<uint32_t>(window.getWidth()), .height = static_cast<uint32_t>(window.getHeight()) };
		}

		ctx.swapchainCI = VkSwapchainCreateInfoKHR{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			                                        .surface = ctx.surface,
			                                        .minImageCount = ctx.surfaceCaps.minImageCount,
			                                        .imageFormat = ctx.imageFormat,
			                                        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			                                        .imageExtent{ .width = swapchainExtent.width, .height = swapchainExtent.height },
			                                        .imageArrayLayers = 1,
			                                        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			                                        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			                                        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			                                        .presentMode = VK_PRESENT_MODE_FIFO_KHR };
		vkResult = vkCreateSwapchainKHR(ctx.device, &ctx.swapchainCI, nullptr, &ctx.swapchain);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating swapchain ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::getSwapchainImages() {
		auto vkResult = vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.imageCount, nullptr);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error getting swapchain images ({})", static_cast<int>(vkResult));
			return false;
		}
		ctx.swapchainImages.resize(ctx.imageCount);
		vkResult = vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.imageCount, ctx.swapchainImages.data());
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error getting swapchain images ({})", static_cast<int>(vkResult));
			return false;
		}
		ctx.swapchainImageViews.resize(ctx.imageCount);
		for (auto i = 0; i < ctx.imageCount; i++) {
			VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                          .image = ctx.swapchainImages[i],
				                          .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                          .format = ctx.imageFormat,
				                          .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
			vkResult = vkCreateImageView(ctx.device, &viewCI, nullptr, &ctx.swapchainImageViews[i]);
			if (vkResult != VK_SUCCESS) {
				CBK_CORE_ERROR("VulkanRendererAPI: error creating image view {} ({})", i, static_cast<int>(vkResult));
				return false;
			}
		}
		return true;
	}

	bool VulkanRendererAPI::createDepthAttachment() {
		auto& window = cbk::Application::get().getWindow();
		std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		for (VkFormat& format: depthFormatList) {
			VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
			vkGetPhysicalDeviceFormatProperties2(ctx.devices[ctx.deviceIdx], format, &formatProperties);
			if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				ctx.depthFormat = format;
				break;
			}
		}
		CBK_ASSERT(ctx.depthFormat != VK_FORMAT_UNDEFINED, "Depth Format is undefined");
		ctx.depthImageCI = VkImageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = ctx.depthFormat,
			.extent{ .width = static_cast<uint32_t>(window.getWidth()), .height = static_cast<uint32_t>(window.getHeight()), .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		return true;
	}

	bool VulkanRendererAPI::createDepthImage() {
		VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };
		auto vkResult = vmaCreateImage(ctx.allocator, &ctx.depthImageCI, &allocCI, &ctx.depthImage, &ctx.depthImageAllocation, nullptr);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating depth image ({})", static_cast<int>(vkResult));
			return false;
		}
		VkImageViewCreateInfo depthViewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			                               .image = ctx.depthImage,
			                               .viewType = VK_IMAGE_VIEW_TYPE_2D,
			                               .format = ctx.depthFormat,
			                               .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 } };
		vkResult = vkCreateImageView(ctx.device, &depthViewCI, nullptr, &ctx.depthImageView);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating depth image view ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::createSyncObjects() {
		VkFenceCreateInfo fenceCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
		for (auto i = 0; i < k_MaxFramesInFlight; i++) {
			auto vkResult = vkCreateFence(ctx.device, &fenceCI, nullptr, &ctx.fences[i]);
			if (vkResult != VK_SUCCESS) {
				CBK_CORE_ERROR("VulkanRendererAPI: error creating fence ({})", static_cast<int>(vkResult));
				return false;
			}
			vkResult = vkCreateSemaphore(ctx.device, &ctx.semaphoreCI, nullptr, &ctx.imageAcquiredSemaphores[i]);
			if (vkResult != VK_SUCCESS) {
				CBK_CORE_ERROR("VulkanRendererAPI: error creating image_aquired Semaphore ({})", static_cast<int>(vkResult));
				return false;
			}
		}

		return createRenderCompleteSemaphores();
	}

	bool VulkanRendererAPI::createRenderCompleteSemaphores() {
		ctx.renderCompleteSemaphores.resize(ctx.swapchainImages.size());
		for (auto& semaphore: ctx.renderCompleteSemaphores) {
			auto vkResult = vkCreateSemaphore(ctx.device, &ctx.semaphoreCI, nullptr, &semaphore);
			if (vkResult != VK_SUCCESS) {
				CBK_CORE_ERROR("VulkanRendererAPI: error creating render_complete Semaphore ({})", static_cast<int>(vkResult));
				return false;
			}
		}
		return true;
	}

	bool VulkanRendererAPI::createCommandPool() {
		VkCommandPoolCreateInfo commandPoolCI{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			                                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			                                   .queueFamilyIndex = ctx.queueFamily };
		auto vkResult = vkCreateCommandPool(ctx.device, &commandPoolCI, nullptr, &ctx.commandPool);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating Command Pool ({})", static_cast<int>(vkResult));
			return false;
		}
		VkCommandBufferAllocateInfo cbAllocCI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			                                   .commandPool = ctx.commandPool,
			                                   .commandBufferCount = k_MaxFramesInFlight };
		vkResult = vkAllocateCommandBuffers(ctx.device, &cbAllocCI, ctx.commandBuffers.data());
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating Command Pool ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::syncAndAcquire() {
		auto vkResult = vkWaitForFences(ctx.device, 1, &ctx.fences[m_FrameIndex], true, UINT64_MAX);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error waiting for fences ({})", static_cast<int>(vkResult));
			return false;
		}
		vkResult = vkResetFences(ctx.device, 1, &ctx.fences[m_FrameIndex]);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error reseting fences ({})", static_cast<int>(vkResult));
			return false;
		}

		vkResult = vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, ctx.imageAcquiredSemaphores[m_FrameIndex], VK_NULL_HANDLE,
		                                 &m_ImageIndex);
		if (vkResult < VK_SUCCESS) {
			if (vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
				m_UpdateSwapchain = true;
				return false;
			}
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error waiting for fences ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}

	bool VulkanRendererAPI::commitRenderCommands(const Ref<rendering::VertexArray>& vertexArray) {
		auto cb = ctx.commandBuffers[m_FrameIndex];
		auto vkResult = vkResetCommandBuffer(cb, 0);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error reseting command buffer ({})", static_cast<int>(vkResult));
			return false;
		}
		VkCommandBufferBeginInfo cbBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		vkResult = vkBeginCommandBuffer(cb, &cbBI);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error beginning command buffer ({})", static_cast<int>(vkResult));
			return false;
		}
		std::array<VkImageMemoryBarrier2, 2> outputBarriers{
			VkImageMemoryBarrier2{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                       .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                       .srcAccessMask = 0,
			                       .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                       .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                       .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                       .image = ctx.swapchainImages[m_ImageIndex],
			                       .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } },
			VkImageMemoryBarrier2{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                       .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			                       .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			                       .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			                       .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			                       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			                       .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                       .image = ctx.depthImage,
			                       .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			                                          .levelCount = 1,
			                                          .layerCount = 1 } }
		};
		VkDependencyInfo barrierDependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                                    .imageMemoryBarrierCount = 2,
			                                    .pImageMemoryBarriers = outputBarriers.data() };
		vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
		VkRenderingAttachmentInfo colorAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = ctx.swapchainImageViews[m_ImageIndex],
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			                                           .clearValue{ .color{ 0.0f, 0.0f, 0.0f, 1.0f } } };
		VkRenderingAttachmentInfo depthAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			                                           .imageView = ctx.depthImageView,
			                                           .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                           .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			                                           .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			                                           .clearValue = { .depthStencil = { 1.0f, 0 } } };

		auto& window = Application::get().getWindow();
		VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			                           .renderArea{ .extent{ .width = static_cast<uint32_t>(window.getWidth()),
			                                                 .height = static_cast<uint32_t>(window.getHeight()) } },
			                           .layerCount = 1,
			                           .colorAttachmentCount = 1,
			                           .pColorAttachments = &colorAttachmentInfo,
			                           .pDepthAttachment = &depthAttachmentInfo };
		vkCmdBeginRendering(cb, &renderingInfo);
		VkViewport vp{ .width = static_cast<float>(window.getWidth()),
			           .height = static_cast<float>(window.getHeight()),
			           .minDepth = 0.0f,
			           .maxDepth = 1.0f };
		vkCmdSetViewport(cb, 0, 1, &vp);
		VkRect2D scissor{ .extent{ .width = static_cast<uint32_t>(window.getWidth()),
			                       .height = static_cast<uint32_t>(window.getHeight()) } };
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline);
		vkCmdSetScissor(cb, 0, 1, &scissor);
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLayout, 0, 1, &ctx.descriptorSetTex, 0, nullptr);
		VkDeviceSize vOffset{ 0 };
		// DO THIS IN VERTEXARRAY AND SHADER?
		// vkCmdBindVertexBuffers(cb, 0, 1, &ctx.vBuffer, &vOffset);
		// vkCmdBindIndexBuffer(cb, ctx.vBuffer, ctx.vBufSize, VK_INDEX_TYPE_UINT16);
		// vkCmdPushConstants(cb, ctx.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress),
		//                    &ctx.shaderDataBuffers[m_FrameIndex].deviceAddress);

		vkCmdDrawIndexed(cb, vertexArray->getIndexBuffer()->getCount(), 3, 0, 0, 0);
		vkCmdEndRendering(cb);
		VkImageMemoryBarrier2 barrierPresent{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			                                  .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                  .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                                  .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			                                  .dstAccessMask = 0,
			                                  .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			                                  .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			                                  .image = ctx.swapchainImages[m_ImageIndex],
			                                  .subresourceRange{
			                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		VkDependencyInfo barrierPresentDependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                                           .imageMemoryBarrierCount = 1,
			                                           .pImageMemoryBarriers = &barrierPresent };
		vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
		vkResult = vkEndCommandBuffer(cb);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error ending command buffer ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}
	bool VulkanRendererAPI::submitQueue() {
		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &ctx.imageAcquiredSemaphores[m_FrameIndex],
			.pWaitDstStageMask = &waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &ctx.commandBuffers[m_FrameIndex],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &ctx.renderCompleteSemaphores[m_ImageIndex],
		};
		auto vkResult = vkQueueSubmit(ctx.queue, 1, &submitInfo, ctx.fences[m_FrameIndex]);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error submitting queue ({})", static_cast<int>(vkResult));
			return false;
		}
		m_FrameIndex = (m_FrameIndex + 1) % k_MaxFramesInFlight;
		VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			                          .waitSemaphoreCount = 1,
			                          .pWaitSemaphores = &ctx.renderCompleteSemaphores[m_ImageIndex],
			                          .swapchainCount = 1,
			                          .pSwapchains = &ctx.swapchain,
			                          .pImageIndices = &m_ImageIndex };
		vkResult = vkQueuePresentKHR(ctx.queue, &presentInfo);
		if (vkResult != VK_SUCCESS) {
			if (vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
				m_UpdateSwapchain = true;
				return false;
			}
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error presenting queue ({})", static_cast<int>(vkResult));
			return false;
		}
		return true;
	}
	void VulkanRendererAPI::updateSwapchain() {
		m_UpdateSwapchain = false;

		auto vkResult = vkDeviceWaitIdle(ctx.device);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error on device wait idle ({})", static_cast<int>(vkResult));
			return;
		}

		// Is this just to see if there is an error?
		vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.devices[ctx.deviceIdx], ctx.surface, &ctx.surfaceCaps);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error getting surface capabilities ({})", static_cast<int>(vkResult));
			return;
		}

		auto& window = cbk::Application::get().getWindow();
		ctx.swapchainCI.oldSwapchain = ctx.swapchain;
		ctx.swapchainCI.imageExtent = { .width = static_cast<uint32_t>(window.getWidth()),
			                            .height = static_cast<uint32_t>(window.getHeight()) };
		vkResult = vkCreateSwapchainKHR(ctx.device, &ctx.swapchainCI, nullptr, &ctx.swapchain);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating swapchain ({})", static_cast<int>(vkResult));
			return;
		}

		for (auto i = 0; i < ctx.imageCount; i++)
			vkDestroyImageView(ctx.device, ctx.swapchainImageViews[i], nullptr);

		if (getSwapchainImages())
			return;

		for (auto& semaphore: ctx.renderCompleteSemaphores)
			vkDestroySemaphore(ctx.device, semaphore, nullptr);

		if (createRenderCompleteSemaphores())
			return;

		vkDestroySwapchainKHR(ctx.device, ctx.swapchainCI.oldSwapchain, nullptr);
		vmaDestroyImage(ctx.allocator, ctx.depthImage, ctx.depthImageAllocation);
		vkDestroyImageView(ctx.device, ctx.depthImageView, nullptr);

		ctx.depthImageCI.extent = { .width = static_cast<uint32_t>(window.getWidth()),
			                        .height = static_cast<uint32_t>(window.getHeight()),
			                        .depth = 1 };
		createDepthImage();
	}
} // namespace cbk::platform::vk
