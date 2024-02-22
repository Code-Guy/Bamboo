#include "pick_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/resource/shader/shader_manager.h"
#include "engine/resource/asset/base/mesh.h"
#include "engine/platform/timer/timer.h"
#include "engine/core/event/event_system.h"

#include <limits>

#define MAX_SIZE 512u

namespace Bamboo
{

	PickPass::PickPass()
	{
		m_formats[0] = VK_FORMAT_R8G8B8A8_UNORM;
		m_formats[1] = VulkanRHI::get().getDepthFormat();

		m_enabled = false;
	}

	void PickPass::render()
	{
		StopWatch stop_watch;
		stop_watch.start();

		// render to framebuffer
		VkClearValue clear_values[2];
		clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clear_values[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo render_pass_bi{};
		render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_bi.renderPass = m_render_pass;
		render_pass_bi.renderArea.extent.width = m_width;
		render_pass_bi.renderArea.extent.height = m_height;
		render_pass_bi.clearValueCount = 2;
		render_pass_bi.pClearValues = clear_values;
		render_pass_bi.framebuffer = m_framebuffer;

		VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();

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

		// render meshes
		uint32_t entity_index = 0;
		for (const auto& render_data : m_render_datas)
		{
			std::shared_ptr<SkeletalMeshRenderData> skeletal_mesh_render_data = nullptr;
			std::shared_ptr<StaticMeshRenderData> static_mesh_render_data = std::static_pointer_cast<StaticMeshRenderData>(render_data);
			bool is_skeletal_mesh = render_data->type == ERenderDataType::SkeletalMesh;;
			if (is_skeletal_mesh)
			{
				skeletal_mesh_render_data = std::static_pointer_cast<SkeletalMeshRenderData>(render_data);
			}

			uint32_t pipeline_index = (uint32_t)is_skeletal_mesh;
			VkPipeline pipeline = m_pipelines[pipeline_index];
			VkPipelineLayout pipeline_layout = m_pipeline_layouts[pipeline_index];

			// bind pipeline
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// bind vertex and index buffer
			VkBuffer vertexBuffers[] = { static_mesh_render_data->vertex_buffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(command_buffer, static_mesh_render_data->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			// render all sub meshes
			std::vector<uint32_t>& index_counts = static_mesh_render_data->index_counts;
			std::vector<uint32_t>& index_offsets = static_mesh_render_data->index_offsets;
			size_t sub_mesh_count = index_counts.size();
			glm::vec4 color = encodeEntityID(m_entity_ids[entity_index++]);
			for (size_t i = 0; i < sub_mesh_count; ++i)
			{
				// update(push) sub mesh descriptors
				std::vector<VkWriteDescriptorSet> desc_writes;
				std::array<VkDescriptorBufferInfo, 2> desc_buffer_infos{};

				// push constants
				updatePushConstants(command_buffer, pipeline_layout, { &color });

				// transform ubo
				addBufferDescriptorSet(desc_writes, desc_buffer_infos[0], static_mesh_render_data->transform_ub, 0);

				// bone matrix ubo
				if (is_skeletal_mesh)
				{
					addBufferDescriptorSet(desc_writes, desc_buffer_infos[1], skeletal_mesh_render_data->bone_ub, 1);
				}

				VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline_layout, 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());

				// render sub mesh
				vkCmdDrawIndexed(command_buffer, index_counts[i], 1, index_offsets[i], 0, 0);
			}
		}

		// render billboards
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[2]);
		for (const auto& render_data : m_billboard_render_datas)
		{
			// push constants
			glm::vec4 color = encodeEntityID(m_entity_ids[entity_index++]);
			updatePushConstants(command_buffer, m_pipeline_layouts[2], { &render_data->position, &color }, m_billboard_push_constant_ranges);

			vkCmdDraw(command_buffer, 1, 1, 0, 0);
		}

		vkCmdEndRenderPass(command_buffer);

		VulkanUtil::endInstantCommands(command_buffer);

		std::vector<uint8_t> image_data;
		VulkanUtil::extractImage(m_color_image_view.image(), m_width, m_height, m_formats[0], image_data);

		uint32_t entity_id = decodeEntityID(&image_data[(m_mouse_y * m_width + m_mouse_x) * 4]);
		g_engine.eventSystem()->asyncDispatch(std::make_shared<SelectEntityEvent>(entity_id));

		//LOG_INFO("pick entity {} elapsed time : {}ms", entity_id, stop_watch.stopMs());
		m_enabled = false;
	}

	void PickPass::createRenderPass()
	{
		// color attachment
		std::array<VkAttachmentDescription, 2> attachments = {};
		attachments[0].format = m_formats[0];
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		// depth attachment
		attachments[1].format = m_formats[1];
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference depth_reference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		// subpass
		VkSubpassDescription subpass_desc{};
		subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_desc.colorAttachmentCount = 1;
		subpass_desc.pColorAttachments = &color_reference;
		subpass_desc.pDepthStencilAttachment = &depth_reference;

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = 2;
		render_pass_ci.pAttachments = attachments.data();
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass_desc;
		render_pass_ci.dependencyCount = 0;
		render_pass_ci.pDependencies = nullptr;

		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create render pass");
	}

	void PickPass::createDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
		};

		VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
		desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();

		m_desc_set_layouts.resize(2);
		VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create static mesh/billboard descriptor set layout");

		desc_set_layout_bindings.insert(desc_set_layout_bindings.begin(), { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[1]);
		CHECK_VULKAN_RESULT(result, "create skeletal mesh descriptor set layout");
	}

	void PickPass::createPipelineLayouts()
	{
		m_push_constant_ranges =
		{
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec4) }
		};

		VkPipelineLayoutCreateInfo pipeline_layout_ci{};
		pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_ci.setLayoutCount = 1;
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[0];
		pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
		pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();

		m_pipeline_layouts.resize(3);
		VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create static mesh pipeline layout");

		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[1];
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[1]);
		CHECK_VULKAN_RESULT(result, "create skeletal mesh pipeline layout");

		// billboard pipeline layouts
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[0];
		m_billboard_push_constant_ranges = {
			{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(glm::vec4) + sizeof(glm::vec2) },
			{ VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::vec4) * 2, sizeof(glm::vec4) }
		};
		pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_billboard_push_constant_ranges.size());
		pipeline_layout_ci.pPushConstantRanges = m_billboard_push_constant_ranges.data();

		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[2]);
		CHECK_VULKAN_RESULT(result, "create billboard pipeline layout");
	}

	void PickPass::createPipelines()
	{
		// vertex input state
		// vertex bindings
		std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions;
		vertex_input_binding_descriptions.resize(1, VkVertexInputBindingDescription{});
		vertex_input_binding_descriptions[0].binding = 0;
		vertex_input_binding_descriptions[0].stride = sizeof(StaticVertex);
		vertex_input_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// static mesh vertex attributes
		std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions;
		vertex_input_attribute_descriptions.resize(3, VkVertexInputAttributeDescription{});

		vertex_input_attribute_descriptions[0].binding = 0;
		vertex_input_attribute_descriptions[0].location = 0;
		vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_input_attribute_descriptions[0].offset = offsetof(StaticVertex, m_position);

		vertex_input_attribute_descriptions[1].binding = 0;
		vertex_input_attribute_descriptions[1].location = 1;
		vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_input_attribute_descriptions[1].offset = offsetof(StaticVertex, m_tex_coord);

		vertex_input_attribute_descriptions[2].binding = 0;
		vertex_input_attribute_descriptions[2].location = 2;
		vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_input_attribute_descriptions[2].offset = offsetof(StaticVertex, m_normal);

		VkPipelineVertexInputStateCreateInfo vertex_input_ci{};
		vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_ci.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_binding_descriptions.size());
		vertex_input_ci.pVertexBindingDescriptions = vertex_input_binding_descriptions.data();
		vertex_input_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
		vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

		// shader stages
		const auto& shader_manager = g_engine.shaderManager();
		std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
			shader_manager->getShaderStageCI("static_mesh.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("pick_mesh.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// create graphics pipeline
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.pVertexInputState = &vertex_input_ci;
		m_pipeline_ci.layout = m_pipeline_layouts[0];
		m_pipeline_ci.renderPass = m_render_pass;
		m_pipeline_ci.subpass = 0;

		m_pipelines.resize(3);
		VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[0]);
		CHECK_VULKAN_RESULT(result, "create pick pass's static mesh graphics pipeline");

		// skeletal mesh vertex attributes
		vertex_input_binding_descriptions[0].stride = sizeof(SkeletalVertex);

		vertex_input_attribute_descriptions.resize(5);
		vertex_input_attribute_descriptions[3].binding = 0;
		vertex_input_attribute_descriptions[3].location = 3;
		vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SINT;
		vertex_input_attribute_descriptions[3].offset = sizeof(StaticVertex);

		vertex_input_attribute_descriptions[4].binding = 0;
		vertex_input_attribute_descriptions[4].location = 4;
		vertex_input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_input_attribute_descriptions[4].offset = sizeof(StaticVertex) + sizeof(glm::ivec4);

		vertex_input_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
		vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

		m_pipeline_ci.layout = m_pipeline_layouts[1];
		shader_stage_cis[0] = shader_manager->getShaderStageCI("skeletal_mesh.vert", VK_SHADER_STAGE_VERTEX_BIT);
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[1]);
		CHECK_VULKAN_RESULT(result, "create pick pass's static mesh graphics pipeline");

		// billboard pipeline
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
		m_input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		// shader stages
		shader_stage_cis = {
			shader_manager->getShaderStageCI("billboard.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("billboard.geom", VK_SHADER_STAGE_GEOMETRY_BIT),
			shader_manager->getShaderStageCI("pick_billboard.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkPipelineVertexInputStateCreateInfo billboard_vertex_input_ci{};
		billboard_vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		m_pipeline_ci.pVertexInputState = &billboard_vertex_input_ci;
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.layout = m_pipeline_layouts[2];
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[2]);
		CHECK_VULKAN_RESULT(result, "create billboard graphics pipeline");
	}

	void PickPass::createFramebuffer()
	{
		// 1.create color/depth images and view
		VulkanUtil::createImageAndView(m_width, m_height, 1, 1, VK_SAMPLE_COUNT_1_BIT, m_formats[0],
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			VK_IMAGE_ASPECT_COLOR_BIT, m_color_image_view);
		VulkanUtil::createImageAndView(m_width, m_height, 1, 1, VK_SAMPLE_COUNT_1_BIT, m_formats[1],
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			VK_IMAGE_ASPECT_DEPTH_BIT, m_depth_image_view);

		// 2.create framebuffer
		std::array<VkImageView, 2> attachments = {
			m_color_image_view.view,
			m_depth_image_view.view
		};

		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = 2;
		framebuffer_ci.pAttachments = attachments.data();
		framebuffer_ci.width = m_width;
		framebuffer_ci.height = m_height;
		framebuffer_ci.layers = 1;

		VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
		CHECK_VULKAN_RESULT(result, "create pick pass frame buffer");
	}

	void PickPass::createResizableObjects(uint32_t width, uint32_t height)
	{
		if (width > height)
		{
			m_width = std::min(width, MAX_SIZE);
			m_height = m_width * height / width;
			m_scale_ratio = (float)m_width / width;
		}
		else
		{
			m_height = std::min(height, MAX_SIZE);
			m_width = m_height * width / height;
			m_scale_ratio = (float)m_height / height;
		}

		createFramebuffer();
	}

	void PickPass::destroyResizableObjects()
	{
		m_color_image_view.destroy();
		m_depth_image_view.destroy();

		RenderPass::destroyResizableObjects();
	}

	void PickPass::pick(uint32_t mouse_x, uint32_t mouse_y)
	{
		m_mouse_x = (uint32_t)(mouse_x * m_scale_ratio);
		m_mouse_y = (uint32_t)(mouse_y * m_scale_ratio);

		if (m_mouse_x < m_width && m_mouse_y < m_height)
		{
			m_enabled = true;
		}
	}

	bool PickPass::isEnabled()
	{
		return RenderPass::isEnabled() && m_enabled;
	}

	glm::vec4 PickPass::encodeEntityID(uint32_t id)
	{
		glm::vec4 color;
		id += 1;
		color.r = (id & 0xFF) / 255.0;
		color.g = ((id >> 8) & 0xFF) / 255.0;
		color.b = ((id >> 16) & 0xFF) / 255.0;

		return color;
	}

	uint32_t PickPass::decodeEntityID(const uint8_t* color)
	{
		uint32_t id;
		id = color[0] + (color[1] << 8) + (color[2] << 16) - 1;
		return id;
	}

}