#include "render_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	void RenderPass::init()
	{
		createRenderPass();
		createDescriptorSetLayouts();
		createPipelineLayouts();
		createPipelineCache();
		createPipelines();
	}

	void RenderPass::destroy()
	{
		if (m_render_pass)
		{
			vkDestroyRenderPass(VulkanRHI::get().getDevice(), m_render_pass, nullptr);
		}
		
		if (m_descriptor_pool)
		{
			vkDestroyDescriptorPool(VulkanRHI::get().getDevice(), m_descriptor_pool, nullptr);
		}
		
		for (VkDescriptorSetLayout desc_set_layout : m_desc_set_layouts)
		{
			vkDestroyDescriptorSetLayout(VulkanRHI::get().getDevice(), desc_set_layout, nullptr);
		}
		for (VkPipelineLayout pipeline_layout : m_pipeline_layouts)
		{
			vkDestroyPipelineLayout(VulkanRHI::get().getDevice(), pipeline_layout, nullptr);
		}
		vkDestroyPipelineCache(VulkanRHI::get().getDevice(), m_pipeline_cache, nullptr);
		for (VkPipeline pipeline : m_pipelines)
		{
			vkDestroyPipeline(VulkanRHI::get().getDevice(), pipeline, nullptr);
		}

		destroyResizableObjects();
	}

	void RenderPass::createPipelineCache()
	{
		// create pipeline cache
		VkPipelineCacheCreateInfo pipeline_cache_ci{};
		pipeline_cache_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkCreatePipelineCache(VulkanRHI::get().getDevice(), &pipeline_cache_ci, nullptr, &m_pipeline_cache);

		// create pipeline create info
		// input assembly
		m_input_assembly_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		m_input_assembly_state_ci.primitiveRestartEnable = VK_FALSE;

		// rasterizer
		m_rasterize_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_rasterize_state_ci.depthClampEnable = VK_FALSE;
		m_rasterize_state_ci.rasterizerDiscardEnable = VK_FALSE;
		m_rasterize_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
		m_rasterize_state_ci.lineWidth = 1.0f;
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
		m_rasterize_state_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		m_rasterize_state_ci.depthBiasEnable = VK_FALSE;
		m_rasterize_state_ci.depthBiasConstantFactor = 0.0f;
		m_rasterize_state_ci.depthBiasClamp = 0.0f;
		m_rasterize_state_ci.depthBiasSlopeFactor = 0.0f;

		// multisampling
		m_multisampling_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_multisampling_ci.sampleShadingEnable = VK_TRUE;
		m_multisampling_ci.minSampleShading = 0.2f;
		m_multisampling_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// depth and stencil testing
		m_depth_stencil_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_depth_stencil_ci.depthTestEnable = VK_TRUE;
		m_depth_stencil_ci.depthWriteEnable = VK_TRUE;
		m_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;
		m_depth_stencil_ci.depthBoundsTestEnable = VK_FALSE;

		// color blending
		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
		m_color_blend_attachments.push_back(color_blend_attachment);

		m_color_blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_color_blend_ci.attachmentCount = static_cast<uint32_t>(m_color_blend_attachments.size());
		m_color_blend_ci.pAttachments = m_color_blend_attachments.data();

		// viewport
		m_viewport_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_viewport_ci.viewportCount = 1;
		m_viewport_ci.pViewports = nullptr;
		m_viewport_ci.scissorCount = 1;
		m_viewport_ci.pScissors = nullptr;

		// Dynamic state
		m_dynamic_states =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		m_dynamic_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		m_dynamic_state_ci.dynamicStateCount = static_cast<uint32_t>(m_dynamic_states.size());
		m_dynamic_state_ci.pDynamicStates = m_dynamic_states.data();

		m_pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		m_pipeline_ci.pInputAssemblyState = &m_input_assembly_state_ci;
		m_pipeline_ci.pViewportState = &m_viewport_ci;
		m_pipeline_ci.pRasterizationState = &m_rasterize_state_ci;
		m_pipeline_ci.pMultisampleState = &m_multisampling_ci;
		m_pipeline_ci.pDepthStencilState = &m_depth_stencil_ci;
		m_pipeline_ci.pColorBlendState = &m_color_blend_ci;
		m_pipeline_ci.pDynamicState = &m_dynamic_state_ci;
	}

	void RenderPass::createResizableObjects(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;

		createFramebuffer();
	}

	void RenderPass::destroyResizableObjects()
	{
		if (m_framebuffer)
		{
			vkDestroyFramebuffer(VulkanRHI::get().getDevice(), m_framebuffer, nullptr);
		}
	}

	void RenderPass::onResize(uint32_t width, uint32_t height)
	{
		// ensure all device operations have done
		VulkanRHI::get().waitDeviceIdle();

		destroyResizableObjects();
		createResizableObjects(width, height);
	}

	bool RenderPass::isEnabled()
	{
		return m_width > 0 && m_height > 0;
	}

	void RenderPass::updatePushConstants(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout, const std::vector<const void*>& pcos)
	{
		for (size_t c = 0; c < m_push_constant_ranges.size(); ++c)
		{
			const VkPushConstantRange& push_constant_range = m_push_constant_ranges[c];
			vkCmdPushConstants(command_buffer, pipeline_layout, push_constant_range.stageFlags, push_constant_range.offset, push_constant_range.size, pcos[c]);
		}
	}

	void RenderPass::addBufferDescriptorSet(std::vector<VkWriteDescriptorSet>& desc_writes,
		VkDescriptorBufferInfo& desc_buffer_info, VmaBuffer buffer, uint32_t binding)
	{
		desc_buffer_info.buffer = buffer.buffer;
		desc_buffer_info.offset = 0;
		desc_buffer_info.range = buffer.size;

		VkWriteDescriptorSet desc_write{};
		desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc_write.dstSet = 0;
		desc_write.dstBinding = binding;
		desc_write.dstArrayElement = 0;
		desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc_write.descriptorCount = 1;
		desc_write.pBufferInfo = &desc_buffer_info;
		desc_writes.push_back(desc_write);
	}

	void RenderPass::addImageDescriptorSet(std::vector<VkWriteDescriptorSet>& desc_writes, 
		VkDescriptorImageInfo& desc_image_info, VmaImageViewSampler texture, uint32_t binding)
	{
		desc_image_info.imageLayout = texture.image_layout;
		desc_image_info.imageView = texture.view;
		desc_image_info.sampler = texture.sampler;

		VkWriteDescriptorSet desc_write{};
		desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc_write.dstSet = 0;
		desc_write.dstBinding = binding;
		desc_write.dstArrayElement = 0;
		desc_write.descriptorType = texture.descriptor_type;
		desc_write.descriptorCount = 1;
		desc_write.pImageInfo = &desc_image_info;
		desc_writes.push_back(desc_write);
	}

	void RenderPass::addImagesDescriptorSet(std::vector<VkWriteDescriptorSet>& desc_writes, 
		VkDescriptorImageInfo* p_desc_image_info, const std::vector<VmaImageViewSampler>& textures, uint32_t binding)
	{
		for (size_t i = 0; i < textures.size(); ++i)
		{
			p_desc_image_info[i].imageLayout = textures[i].image_layout;
			p_desc_image_info[i].imageView = textures[i].view;
			p_desc_image_info[i].sampler = textures[i].sampler;
		}

		VkWriteDescriptorSet desc_write{};
		desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc_write.dstSet = 0;
		desc_write.dstBinding = binding;
		desc_write.dstArrayElement = 0;
		desc_write.descriptorType = textures.front().descriptor_type;
		desc_write.descriptorCount = static_cast<uint32_t>(textures.size());
		desc_write.pImageInfo = p_desc_image_info;
		desc_writes.push_back(desc_write);
	}

}