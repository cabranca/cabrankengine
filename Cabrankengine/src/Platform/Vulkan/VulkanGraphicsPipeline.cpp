#include <pch.h>
#include "VulkanGraphicsPipeline.h"

#include <Common/BinaryFormats.h>

#include "VkCheck.h"
#include "VulkanCommands.h"
#include "VulkanShader.h"

namespace cbk::platform::vk {

	void VulkanGraphicsPipeline::init(const VulkanDeviceContext& ctx) {
		m_Device = ctx.getDevice();
		// The UBO owns set 0's layout, so it has to exist before createPipelineLayout()
		// reads it. TODO: see how to share this between Phong and PBR (static?)
		m_UBO.init(ctx, sizeof(SceneData), k_SceneDataBinding, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSet();
		createPipelineLayout();
		createPipeline(ctx.getImageFormat(), ctx.getDepthFormat(), ctx.getMSAA());
	}

	void VulkanGraphicsPipeline::shutdown() {
		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_SetLayout, nullptr);
		m_UBO.shutdown();
	}

	void VulkanGraphicsPipeline::bind(VkCommandBuffer cb, const math::Mat4& transform, float shininess, uint32_t frameIndex) {
		PushData pushData{ .transform = transform, .shininess = shininess };
		std::array<VkDescriptorSet, 2> sets = { m_UBO.getDescriptorSet(frameIndex), m_DescriptorSet };
		VulkanCommands::bindPipeline(cb, m_Pipeline, m_PipelineLayout,
		                             { .FirstSet = 0, .DescriptorSetCount = sets.size(), .DescriptorSets = sets.data() },
		                             { .StageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		                               .Offset = 0,
		                               .Size = sizeof(PushData),
		                               .Values = &pushData });
	}

	void VulkanGraphicsPipeline::createDescriptorSetLayout() {
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

		VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &dslCI, nullptr, &m_SetLayout));
	}

	void VulkanGraphicsPipeline::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(k_MaterialBindingCount) * k_MaxInstances,
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = k_MaxInstances,
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		VK_CHECK(vkCreateDescriptorPool(m_Device, &poolCI, nullptr, &m_DescriptorPool));
	}

	void VulkanGraphicsPipeline::createDescriptorSet() {
		VkDescriptorSetAllocateInfo dsAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_DescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &m_SetLayout,
		};
		VK_CHECK(vkAllocateDescriptorSets(m_Device, &dsAllocInfo, &m_DescriptorSet));
	}

	void VulkanGraphicsPipeline::createPipelineLayout() {
		std::array<VkDescriptorSetLayout, 2> setLayouts = { m_UBO.getDescriptorSetLayout(), m_SetLayout };

		// Slang emits one push-constant block visible to both stages, so use a single
		// range covering the whole block instead of splitting per stage.
		std::array<VkPushConstantRange, 1> pushRanges = {
			VkPushConstantRange{
			    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			    .offset = 0,
			    .size = static_cast<uint32_t>(sizeof(PushData)),
			},
		};

		VkPipelineLayoutCreateInfo plCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
			.pSetLayouts = setLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(pushRanges.size()),
			.pPushConstantRanges = pushRanges.data(),
		};
		VK_CHECK(vkCreatePipelineLayout(m_Device, &plCI, nullptr, &m_PipelineLayout));
	}

	void VulkanGraphicsPipeline::createPipeline(VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount) {
		rendering::ShaderLibrary::load("assets/shaders/Phong");
		auto shader = static_cast<VulkanShader*>(rendering::ShaderLibrary::get("Phong").get());

		std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
			VkPipelineShaderStageCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			                                 .stage = VK_SHADER_STAGE_VERTEX_BIT,
			                                 .module = shader->getModule(),
			                                 .pName = "main" },
			VkPipelineShaderStageCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			                                 .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			                                 .module = shader->getModule(),
			                                 .pName = "main" },
		};

		VkVertexInputBindingDescription vertexBinding{
			.binding = 0,
			.stride = sizeof(common::Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		std::array<VkVertexInputAttributeDescription, 4> vertexAttributes = {
			VkVertexInputAttributeDescription{
			    .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(common::Vertex, position) },
			VkVertexInputAttributeDescription{
			    .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(common::Vertex, normal) },
			VkVertexInputAttributeDescription{
			    .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(common::Vertex, texCoords) },
			VkVertexInputAttributeDescription{
			    .location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(common::Vertex, tangent) },
		};

		VkPipelineVertexInputStateCreateInfo vertexInput{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			                                              .vertexBindingDescriptionCount = 1,
			                                              .pVertexBindingDescriptions = &vertexBinding,
			                                              .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
			                                              .pVertexAttributeDescriptions = vertexAttributes.data() };

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			                                                  .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			                                                  .primitiveRestartEnable = VK_FALSE };

		VkPipelineViewportStateCreateInfo viewportState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			                                             .viewportCount = 1,
			                                             .pViewports = nullptr,
			                                             .scissorCount = 1,
			                                             .pScissors = nullptr };

		VkPipelineRasterizationStateCreateInfo rasterization{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			                                                  .depthClampEnable = VK_FALSE,
			                                                  .rasterizerDiscardEnable = VK_FALSE,
			                                                  .polygonMode = VK_POLYGON_MODE_FILL,
			                                                  .cullMode = VK_CULL_MODE_BACK_BIT,
			                                                  .frontFace = VK_FRONT_FACE_CLOCKWISE,
			                                                  .depthBiasEnable = VK_FALSE,
			                                                  .depthBiasConstantFactor = 0.f,
			                                                  .depthBiasClamp = 0.f,
			                                                  .depthBiasSlopeFactor = 0.f,
			                                                  .lineWidth = 1.0f };

		VkPipelineMultisampleStateCreateInfo multisample{ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			                                              .rasterizationSamples = sampleCount,
			                                              .sampleShadingEnable = VK_TRUE,
			                                              .minSampleShading = 0.2f,
			                                              .pSampleMask = nullptr,
			                                              .alphaToCoverageEnable = VK_FALSE,
			                                              .alphaToOneEnable = VK_FALSE };

		VkPipelineDepthStencilStateCreateInfo depthStencil{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			                                                .depthTestEnable = VK_TRUE,
			                                                .depthWriteEnable = VK_TRUE,
			                                                .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			                                                .depthBoundsTestEnable = VK_FALSE,
			                                                .stencilTestEnable = VK_FALSE,
			                                                .front = {},
			                                                .back = {},
			                                                .minDepthBounds = 0.f,
			                                                .maxDepthBounds = 0.f };

		// TODO: research on this
		VkPipelineColorBlendAttachmentState blendAttachment{ .colorWriteMask = 0xF };
		VkPipelineColorBlendStateCreateInfo colorBlend{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			//.logicOpEnable = VK_FALSE,
			//.logicOp = VK_LOGIC_OP_AND,
			.attachmentCount = 1,
			.pAttachments = &blendAttachment,
		};

		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data(),
		};

		VkPipelineRenderingCreateInfo rendering{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &colorFormat,
			.depthAttachmentFormat = depthFormat,
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
			.layout = m_PipelineLayout,
		};
		VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_Pipeline));
	}

	VulkanUniformBuffer& VulkanGraphicsPipeline::getUBO() {
		return m_UBO;
	}

	VkDescriptorSet VulkanGraphicsPipeline::getDescriptorSet() const {
		return m_DescriptorSet;
	}
} // namespace cbk::platform::vk
