#include <pch.h>
#include "VulkanPhongMaterial.h"

#include <array>

#include <Cabrankengine/Renderer/GeometryDescriptor.h>
#include <Cabrankengine/Renderer/Renderer.h>
#include <Cabrankengine/Renderer/Shader.h>

#include "VkCheck.h"
#include "VulkanDescriptorBinding.h"
#include "VulkanDeviceContext.h"
#include "VulkanRendererAPI.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"

namespace cbk::platform::vk {

	using namespace rendering;

	static constexpr uint32_t k_MaxInstances = 256;

	bool VulkanPhongMaterial::s_Initialized = false;
	VkDescriptorSetLayout VulkanPhongMaterial::s_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool VulkanPhongMaterial::s_DescriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout VulkanPhongMaterial::s_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline VulkanPhongMaterial::s_Pipeline = VK_NULL_HANDLE;

	VulkanPhongMaterial::VulkanPhongMaterial() : PhongMaterial() {
		
		auto ctx = &VulkanRendererAPI::getContext();
		s_MSAA = ctx->getMSAA();
		initSharedResourcesIfNeeded();
		VkDescriptorSetAllocateInfo dsAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = s_DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &s_DescriptorSetLayout,
		};
		VK_CHECK(vkAllocateDescriptorSets(ctx->getDevice(), &dsAllocInfo, &m_DescriptorSet));
	}

	VulkanPhongMaterial::~VulkanPhongMaterial() {
		// Descriptor set is freed when s_DescriptorPool is destroyed.
	}

	void VulkanPhongMaterial::record(VkCommandBuffer cb, const math::Mat4& transform) const {
		if (m_DescriptorDirty)
			updateDescriptorSet();

		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Pipeline);

		// Set 0 = scene globals, set 1 = this material's textures. No light SSBO set.
		bindSceneSet(cb, s_PipelineLayout);
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, s_PipelineLayout, set_index::k_Material, 1, &m_DescriptorSet, 0,
		                        nullptr);

		// Push constants: [0..64) model matrix, [64..) material params.
		vkCmdPushConstants(cb, s_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(math::Mat4),
		                   &transform);
		PushData pushData{ .shininess = m_Shininess };
		vkCmdPushConstants(cb, s_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(math::Mat4),
		                   sizeof(PushData), &pushData);
	}

	void VulkanPhongMaterial::updateDescriptorSet() const {
		auto ctx = &VulkanRendererAPI::getContext();

		// 1. Get the textures' descriptorsImageInfo
		if (!m_DiffuseMap || !m_SpecularMap) {
			// Defer descriptor write until both texture slots are populated.
			return;
		}

		auto diffuseVk = static_cast<VulkanTexture*>(m_DiffuseMap.get());
		auto specularVk = static_cast<VulkanTexture*>(m_SpecularMap.get());

		std::array<VkDescriptorImageInfo, 2> imageInfos{
			*diffuseVk->getDescriptor(),
			*specularVk->getDescriptor(),
		};

		// 2. Update the descriptor sets writing them.
		std::array<VkWriteDescriptorSet, 2> writes{};
		for (uint32_t i = 0; i < 2; ++i) {
			writes[i] = VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSet,
				.dstBinding = i,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos[i],
			};
		}
		vkUpdateDescriptorSets(ctx->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		m_DescriptorDirty = false;
	}

	void VulkanPhongMaterial::initSharedResourcesIfNeeded() {
		if (s_Initialized)
			return;
		s_Initialized = true;

		auto ctx = &VulkanRendererAPI::getContext();
		auto device = ctx->getDevice();

		// 1) Material descriptor set layout — 2 combined image samplers.
		std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
		for (uint32_t i = 0; i < bindings.size(); ++i) {
			bindings[i] = VkDescriptorSetLayoutBinding{
				.binding = i,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			};
		}
		VkDescriptorSetLayoutCreateInfo dslCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data(),
		};
		VK_CHECK(vkCreateDescriptorSetLayout(device, &dslCI, nullptr, &s_DescriptorSetLayout));

		// 2) Pool.
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(bindings.size()) * k_MaxInstances,
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = k_MaxInstances,
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		VK_CHECK(vkCreateDescriptorPool(device, &poolCI, nullptr, &s_DescriptorPool));

		// 3) Pipeline layout.
		auto sceneUbo = static_cast<VulkanUniformBuffer*>(Renderer::getSceneUBO().get());
		auto sceneLayout = sceneUbo->getDescriptorSetLayout();
		std::array<VkDescriptorSetLayout, 2> setLayouts = { sceneLayout, s_DescriptorSetLayout };

		// Slang emits one push-constant block visible to both stages, so use a single
		// range covering the whole block instead of splitting per stage.
		std::array<VkPushConstantRange, 1> pushRanges = {
			VkPushConstantRange{
			    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			    .offset = 0,
			    .size = static_cast<uint32_t>(sizeof(math::Mat4) + sizeof(PushData)),
			},
		};

		VkPipelineLayoutCreateInfo plCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
			.pSetLayouts = setLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(pushRanges.size()),
			.pPushConstantRanges = pushRanges.data(),
		};
		VK_CHECK(vkCreatePipelineLayout(device, &plCI, nullptr, &s_PipelineLayout));

		// 4) Graphics pipeline.
		auto shader = static_cast<VulkanShader*>(ShaderLibrary::get("Phong").get());
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
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		std::array<VkVertexInputAttributeDescription, 4> vertexAttributes = {
			VkVertexInputAttributeDescription{
			    .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position) },
			VkVertexInputAttributeDescription{
			    .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
			VkVertexInputAttributeDescription{
			    .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, texCoords) },
			VkVertexInputAttributeDescription{
			    .location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, tangent) },
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
			.rasterizationSamples = s_MSAA,
		};
		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		};
		VkPipelineColorBlendAttachmentState blendAttachment{ .colorWriteMask = 0xF };
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
		VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &s_Pipeline));
	}

	void VulkanPhongMaterial::destroySharedResources() {
		if (!s_Initialized)
			return;
		auto ctx = &VulkanRendererAPI::getContext();
		auto device = ctx->getDevice();
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
