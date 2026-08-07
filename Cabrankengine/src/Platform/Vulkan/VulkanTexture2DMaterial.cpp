#include <pch.h>
#include "VulkanTexture2DMaterial.h"

#include <array>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Common/Math/Mat4.h>
#include <Cabrankengine/Renderer/Renderer.h>
#include <Cabrankengine/Renderer/Shader.h>

#include "VulkanDescriptorBinding.h"
#include "VulkanDeviceContext.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"

namespace cbk::platform::vk {

	using namespace rendering;

	// One instance is the common case (a single Renderer2D batcher), but allow a
	// handful so callers that build their own batchers aren't constrained.
	static constexpr uint32_t k_MaxInstances = 4;

	// Must match the QuadVertex layout in Renderer2D.cpp.
	struct Renderer2DVertex {
		float position[3];
		float color[4];
		float texCoord[2];
		float texIndex;
		float tilingFactor;
	};

	bool VulkanTexture2DMaterial::s_Initialized = false;
	VkDescriptorSetLayout VulkanTexture2DMaterial::s_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool VulkanTexture2DMaterial::s_DescriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout VulkanTexture2DMaterial::s_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline VulkanTexture2DMaterial::s_Pipeline = VK_NULL_HANDLE;

	VulkanTexture2DMaterial::VulkanTexture2DMaterial() : Texture2DMaterial() {
		initSharedResourcesIfNeeded();

		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		VkDescriptorSetAllocateInfo dsAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = s_DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &s_DescriptorSetLayout,
		};
		auto vkResult = vkAllocateDescriptorSets(ctx->getLogicalDevice(), &dsAllocInfo, &m_DescriptorSet);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture2DMaterial: vkAllocateDescriptorSets failed ({})", static_cast<int>(vkResult));
		}
	}

	VulkanTexture2DMaterial::~VulkanTexture2DMaterial() {
		// Descriptor set is freed when s_DescriptorPool is destroyed.
	}

	void VulkanTexture2DMaterial::setTextureSlot(uint32_t slot, const Ref<Texture2D>& texture) {
		CBK_CORE_ASSERT(slot < k_MaxTextureSlots, "VulkanTexture2DMaterial: texture slot out of range");
		if (m_TextureSlots[slot].get() == texture.get())
			return;
		m_TextureSlots[slot] = texture;
		m_DescriptorDirty = true;
	}

	void VulkanTexture2DMaterial::record(VkCommandBuffer cb, const math::Mat4& /*transform*/) const {
		if (m_DescriptorDirty)
			updateDescriptorSet();

		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Pipeline);

		// Set 0 = scene globals, set 1 = the 32-sampler quad atlas array. Batched quads
		// are already in world space, so there is no model-matrix push constant.
		bindSceneSet(cb, s_PipelineLayout);
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, s_PipelineLayout, set_index::k_Material, 1, &m_DescriptorSet, 0,
		                        nullptr);
	}

	void VulkanTexture2DMaterial::updateDescriptorSet() const {
		// Slot 0 is the white fallback used for un-textured quads and as a stand-in
		// for unused slots — the shader still indexes them so they must be valid.
		CBK_CORE_ASSERT(m_TextureSlots[0], "VulkanTexture2DMaterial: slot 0 must be populated before flush");

		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());

		std::array<VkDescriptorImageInfo, k_MaxTextureSlots> imageInfos{};
		const auto& fallback = m_TextureSlots[0];
		for (uint32_t i = 0; i < k_MaxTextureSlots; ++i) {
			const auto& tex = m_TextureSlots[i] ? m_TextureSlots[i] : fallback;
			imageInfos[i] = *static_cast<VulkanTexture*>(tex.get())->getDescriptor();
		}

		VkWriteDescriptorSet write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = m_DescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = k_MaxTextureSlots,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = imageInfos.data(),
		};
		vkUpdateDescriptorSets(ctx->getLogicalDevice(), 1, &write, 0, nullptr);
		m_DescriptorDirty = false;
	}

	void VulkanTexture2DMaterial::initSharedResourcesIfNeeded() {
		if (s_Initialized)
			return;
		s_Initialized = true;

		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		auto device = ctx->getLogicalDevice();

		// 1) Material descriptor set layout — one binding with a 32-sampler array.
		VkDescriptorSetLayoutBinding binding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = k_MaxTextureSlots,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		};
		VkDescriptorSetLayoutCreateInfo dslCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &binding,
		};
		vkCreateDescriptorSetLayout(device, &dslCI, nullptr, &s_DescriptorSetLayout);

		// 2) Pool — each set consumes k_MaxTextureSlots combined image samplers.
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = k_MaxTextureSlots * k_MaxInstances,
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = k_MaxInstances,
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		vkCreateDescriptorPool(device, &poolCI, nullptr, &s_DescriptorPool);

		// 3) Pipeline layout — set 0 = scene UBO, set 1 = sampler array. The batcher's
		// vertices are already in world space, so there are no push constants: the
		// material's record() never pushes a model matrix.
		auto sceneUbo = static_cast<VulkanUniformBuffer*>(Renderer::getSceneUBO().get());
		auto sceneLayout = sceneUbo->getDescriptorSetLayout();
		std::array<VkDescriptorSetLayout, 2> setLayouts = { sceneLayout, s_DescriptorSetLayout };

		VkPipelineLayoutCreateInfo plCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
			.pSetLayouts = setLayouts.data(),
		};
		vkCreatePipelineLayout(device, &plCI, nullptr, &s_PipelineLayout);

		// 4) Graphics pipeline.
		auto shader = static_cast<VulkanShader*>(ShaderLibrary::get("Texture").get());
		auto shaderModule = shader->getModule();

		std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
			VkPipelineShaderStageCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			                                 .stage = VK_SHADER_STAGE_VERTEX_BIT,
			                                 .module = shaderModule,
			                                 .pName = "main" },
			VkPipelineShaderStageCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			                                 .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			                                 .module = shaderModule,
			                                 .pName = "main" },
		};

		VkVertexInputBindingDescription vertexBinding{
			.binding = 0,
			.stride = sizeof(Renderer2DVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		std::array<VkVertexInputAttributeDescription, 5> vertexAttributes = {
			VkVertexInputAttributeDescription{
			    .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Renderer2DVertex, position) },
			VkVertexInputAttributeDescription{
			    .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Renderer2DVertex, color) },
			VkVertexInputAttributeDescription{
			    .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Renderer2DVertex, texCoord) },
			VkVertexInputAttributeDescription{
			    .location = 3, .binding = 0, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(Renderer2DVertex, texIndex) },
			VkVertexInputAttributeDescription{
			    .location = 4, .binding = 0, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(Renderer2DVertex, tilingFactor) },
		};
		VkPipelineVertexInputStateCreateInfo vertexInput{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertexBinding,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
			.pVertexAttributeDescriptions = vertexAttributes.data(),
		};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};
		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};
		VkPipelineRasterizationStateCreateInfo rasterization{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.lineWidth = 1.0f,
		};
		VkPipelineMultisampleStateCreateInfo multisample{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};
		// 2D batches submit in draw order and rely on painter's algorithm rather
		// than depth — disable depth so per-frame interleaving with 3D doesn't
		// punch holes.
		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_FALSE,
			.depthWriteEnable = VK_FALSE,
		};
		// Premultiplied-like alpha blend so quads with vertex alpha < 1 composite
		// against the existing color attachment.
		VkPipelineColorBlendAttachmentState blendAttachment{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = 0xF,
		};
		VkPipelineColorBlendStateCreateInfo colorBlend{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blendAttachment,
		};
		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data(),
		};

		VkFormat colorFormat = ctx->getImageFormat();
		VkPipelineRenderingCreateInfo rendering{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &colorFormat,
			.depthAttachmentFormat = ctx->getDepthFormat(),
		};

		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &rendering,
			.stageCount = static_cast<uint32_t>(stages.size()),
			.pStages = stages.data(),
			.pVertexInputState = &vertexInput,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterization,
			.pMultisampleState = &multisample,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlend,
			.pDynamicState = &dynamicState,
			.layout = s_PipelineLayout,
		};
		auto vkResult = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &s_Pipeline);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanTexture2DMaterial: vkCreateGraphicsPipelines failed ({})", static_cast<int>(vkResult));
		}
	}

	void VulkanTexture2DMaterial::destroySharedResources() {
		if (!s_Initialized)
			return;
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		auto device = ctx->getLogicalDevice();
		if (s_Pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(device, s_Pipeline, nullptr);
		if (s_PipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(device, s_PipelineLayout, nullptr);
		if (s_DescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(device, s_DescriptorPool, nullptr);
		if (s_DescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(device, s_DescriptorSetLayout, nullptr);
		s_Initialized = false;
	}

} // namespace cbk::platform::vk
