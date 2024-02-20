#include "postprocess_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/resource/shader/shader_manager.h"

namespace Bamboo
{

	PostprocessPass::PostprocessPass()
	{
		m_format = VK_FORMAT_R8G8B8A8_UNORM;
	}

	void PostprocessPass::init()
	{
		RenderPass::init();

		loadColorGradingTexture("asset/engine/texture/postprocess/cg_none.png");
	}

	void PostprocessPass::render()
	{
		std::shared_ptr<PostProcessRenderData> postprocess_render_data = std::static_pointer_cast<PostProcessRenderData>(m_render_datas.front());

		// render to framebuffer
		VkClearValue clear_value;
		clear_value.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo render_pass_bi{};
		render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_bi.renderPass = m_render_pass;
		render_pass_bi.renderArea.extent.width = m_width;
		render_pass_bi.renderArea.extent.height = m_height;
		render_pass_bi.clearValueCount = 1;
		render_pass_bi.pClearValues = &clear_value;
		render_pass_bi.framebuffer = m_framebuffer;

		VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();

		VkViewport viewport{};
		viewport.width = static_cast<float>(m_width);
		viewport.height = static_cast<float>(m_height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent.width = m_width;
		scissor.extent.height = m_height;

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0]);

		std::vector<VkWriteDescriptorSet> desc_writes;
		std::array<VkDescriptorImageInfo, 3> desc_image_infos{};

		// push constants
		int is_selecting = postprocess_render_data->outline_texture != nullptr;
		updatePushConstants(command_buffer, m_pipeline_layouts[0], { &is_selecting });

		// texture image samplers
		addImageDescriptorSet(desc_writes, desc_image_infos[0], *postprocess_render_data->p_color_texture, 0);
		VmaImageViewSampler outline_texture = is_selecting ? *postprocess_render_data->outline_texture : *postprocess_render_data->p_color_texture;
		addImageDescriptorSet(desc_writes, desc_image_infos[1], outline_texture, 1);
		addImageDescriptorSet(desc_writes, desc_image_infos[2], m_color_grading_texture_sampler, 2);

		VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline_layouts[0], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
		vkCmdDraw(command_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(command_buffer);

// 		VulkanUtil::saveImage(m_color_texture_sampler.image(), static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height),
// 			m_format, "D:/Data/Test/main_pass/data.bin", VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void PostprocessPass::destroy()
	{
		m_color_grading_texture_sampler.destroy();

		RenderPass::destroy();
	}

	void PostprocessPass::createRenderPass()
	{
		// color attachment
		VkAttachmentDescription color_attachment{};
		color_attachment.format = m_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		// subpass
		VkSubpassDescription subpass_desc{};
		subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_desc.colorAttachmentCount = 1;
		subpass_desc.pColorAttachments = &color_reference;

		// subpass dependencies
		std::vector<VkSubpassDependency> dependencies =
		{
			{
				VK_SUBPASS_EXTERNAL,
				0,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				0
			},
			{
				0,
				VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				0
			}
		};

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = 1;
		render_pass_ci.pAttachments = &color_attachment;
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass_desc;
		render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
		render_pass_ci.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create postprocess render pass");
	}

	void PostprocessPass::createDescriptorSetLayouts()
	{
		VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
		desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

		std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		m_desc_set_layouts.resize(1);
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create postprocess descriptor set layout");
	}

	void PostprocessPass::createPipelineLayouts()
	{
		m_push_constant_ranges =
		{
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) }
		};

		VkPipelineLayoutCreateInfo pipeline_layout_ci{};
		pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_ci.setLayoutCount = 1;
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[0];
		pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
		pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();

		m_pipeline_layouts.resize(1);
		VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create postprocess pipeline layout");
	}

	void PostprocessPass::createPipelines()
	{
		// disable culling and depth testing
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
		m_depth_stencil_ci.depthTestEnable = VK_FALSE;
		m_depth_stencil_ci.depthWriteEnable = VK_FALSE;
		m_color_blend_attachments[0].blendEnable = VK_FALSE;

		// vertex input state
		VkPipelineVertexInputStateCreateInfo vertex_input_ci{};
		vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// shader stages
		const auto& shader_manager = g_engine.shaderManager();
		std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
			shader_manager->getShaderStageCI("screen.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("outline_color_grading.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		m_pipeline_ci.renderPass = m_render_pass;
		m_pipeline_ci.pVertexInputState = &vertex_input_ci;
		m_pipeline_ci.layout = m_pipeline_layouts[0];
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();

		m_pipelines.resize(1);
		VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[0]);
		CHECK_VULKAN_RESULT(result, "create postprocess graphics pipeline");
	}

	void PostprocessPass::createFramebuffer()
	{
		// 1.create color images and view
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_format,
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_color_texture_sampler,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

		// 2.create framebuffer
		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = 1;
		framebuffer_ci.pAttachments = &m_color_texture_sampler.view;
		framebuffer_ci.width = m_width;
		framebuffer_ci.height = m_height;
		framebuffer_ci.layers = 1;

		VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
		CHECK_VULKAN_RESULT(result, "create postprocess pass frame buffer");
	}

	void PostprocessPass::destroyResizableObjects()
	{
		m_color_texture_sampler.destroy();

		RenderPass::destroyResizableObjects();
	}

	void PostprocessPass::loadColorGradingTexture(const std::string& filename)
	{
		// destroy last texture sampler
		m_color_grading_texture_sampler.destroy();

		// load image data
		uint32_t width;
		uint32_t height;
		std::vector<uint8_t> image_data;
		VulkanUtil::loadImageData(filename, width, height, image_data);
		glm::uvec3 dim = { width, width, height / width };

		// create image
		VkImageCreateInfo image_ci{};
		image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_ci.imageType = VK_IMAGE_TYPE_3D;
		image_ci.extent.width = dim[0];
		image_ci.extent.height = dim[1];
		image_ci.extent.depth = dim[2];
		image_ci.mipLevels = 1;
		image_ci.arrayLayers = 1;
		image_ci.format = m_format;
		image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_ci.samples = VK_SAMPLE_COUNT_1_BIT;

		VmaAllocationCreateInfo vma_alloc_ci{};
		vma_alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		VkResult result = vmaCreateImage(VulkanRHI::get().getAllocator(), &image_ci, &vma_alloc_ci, 
			&m_color_grading_texture_sampler.vma_image.image, &m_color_grading_texture_sampler.vma_image.allocation, nullptr);
		CHECK_VULKAN_RESULT(result, "vma create 3d texture image");

		// create view
		VkImageViewCreateInfo image_view_ci{};
		image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_ci.image = m_color_grading_texture_sampler.image();
		image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_3D;
		image_view_ci.format = m_format;
		image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_ci.subresourceRange.baseMipLevel = 0;
		image_view_ci.subresourceRange.levelCount = 1;
		image_view_ci.subresourceRange.baseArrayLayer = 0;
		image_view_ci.subresourceRange.layerCount = 1;
		vkCreateImageView(VulkanRHI::get().getDevice(), &image_view_ci, nullptr, &m_color_grading_texture_sampler.view);

		// create sampler
		m_color_grading_texture_sampler.sampler = VulkanUtil::createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 1,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		// create staging buffer and copy data into it
		size_t buffer_size = image_data.size();
		VmaBuffer staging_buffer;
		VulkanUtil::createBuffer(buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
			staging_buffer);

		VulkanUtil::updateBuffer(staging_buffer, image_data.data(), buffer_size);
		
		// copy staging buffer to image
		VkBufferImageCopy buffer_copy_region{};
		buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_copy_region.imageSubresource.mipLevel = 0;
		buffer_copy_region.imageSubresource.baseArrayLayer = 0;
		buffer_copy_region.imageSubresource.layerCount = 1;
		buffer_copy_region.imageExtent.width = dim[0];
		buffer_copy_region.imageExtent.height = dim[1];
		buffer_copy_region.imageExtent.depth = dim[2];

		VulkanUtil::transitionImageLayout(m_color_grading_texture_sampler.image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_format);
		VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();
		vkCmdCopyBufferToImage(
			command_buffer,
			staging_buffer.buffer,
			m_color_grading_texture_sampler.image(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&buffer_copy_region);
		VulkanUtil::endInstantCommands(command_buffer);
		vmaDestroyBuffer(VulkanRHI::get().getAllocator(), staging_buffer.buffer, staging_buffer.allocation);

		// transition image layout
		VulkanUtil::transitionImageLayout(m_color_grading_texture_sampler.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_format);
		m_color_grading_texture_sampler.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_color_grading_texture_sampler.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

}