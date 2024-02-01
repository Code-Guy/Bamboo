#include "directional_light_shadow_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/resource/shader/shader_manager.h"
#include "engine/resource/asset/base/mesh.h"
#include "engine/core/math/transform.h"

namespace Bamboo
{

	DirectionalLightShadowPass::DirectionalLightShadowPass()
	{
		m_format = VulkanRHI::get().getDepthFormat();
		m_size = 2048;
		m_cascade_split_lambda = 0.95f;
	}

	void DirectionalLightShadowPass::init()
	{
		RenderPass::init();

		// create shadow cascade uniform buffers
		m_shadow_cascade_ubs.resize(MAX_FRAMES_IN_FLIGHT);
		for (VmaBuffer& uniform_buffer : m_shadow_cascade_ubs)
		{
			VulkanUtil::createBuffer(sizeof(ShadowCascadeUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, uniform_buffer, true);
		}

		createResizableObjects(m_size, m_size);
	}

	void DirectionalLightShadowPass::render()
	{
		VkRenderPassBeginInfo render_pass_bi{};
		render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_bi.renderPass = m_render_pass;
		render_pass_bi.framebuffer = m_framebuffer;
		render_pass_bi.renderArea.offset = { 0, 0 };
		render_pass_bi.renderArea.extent = { m_size, m_size };

		VkClearValue clear_value{};
		clear_value.depthStencil = { 1.0f, 0 };
		render_pass_bi.clearValueCount = 1;
		render_pass_bi.pClearValues = &clear_value;

 		VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();
		vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_size);
		viewport.height = static_cast<float>(m_size);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_size, m_size };
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		for (const auto& render_data : m_render_datas)
		{
			std::shared_ptr<SkeletalMeshRenderData> skeletal_mesh_render_data = nullptr;
			std::shared_ptr<StaticMeshRenderData> static_mesh_render_data = std::static_pointer_cast<StaticMeshRenderData>(render_data);
			bool is_skeletal_mesh = render_data->type == ERenderDataType::SkeletalMesh;
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
			for (size_t i = 0; i < sub_mesh_count; ++i)
			{
				// update(push) sub mesh descriptors
				std::vector<VkWriteDescriptorSet> desc_writes;
				std::array<VkDescriptorBufferInfo, 3> desc_buffer_infos{};
				std::array<VkDescriptorImageInfo, 1> desc_image_infos{};

				// bone matrix ubo
				if (is_skeletal_mesh)
				{
					addBufferDescriptorSet(desc_writes, desc_buffer_infos[0], skeletal_mesh_render_data->bone_ub, 0);
				}
				addBufferDescriptorSet(desc_writes, desc_buffer_infos[1], static_mesh_render_data->transform_ub, 12);

				// shadow cascade ubo
				addBufferDescriptorSet(desc_writes, desc_buffer_infos[2], m_shadow_cascade_ubs[VulkanRHI::get().getFlightIndex()], 1);
	
				// base color texture image sampler
				addImageDescriptorSet(desc_writes, desc_image_infos[0], static_mesh_render_data->pbr_textures[i].base_color_texure, 2);

				VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline_layout, 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());

				// render sub mesh
				vkCmdDrawIndexed(command_buffer, index_counts[i], 1, index_offsets[i], 0, 0);
			}
		}

		vkCmdEndRenderPass(command_buffer);
		m_render_datas.clear();
	}

	void DirectionalLightShadowPass::destroy()
	{
		RenderPass::destroy();

		// destroy shadow cascade uniform buffers
		for (VmaBuffer& uniform_buffer : m_shadow_cascade_ubs)
		{
			uniform_buffer.destroy();
		}
	}

	void DirectionalLightShadowPass::createRenderPass()
	{
		// depth attachment
		VkAttachmentDescription depth_attachment{};
		depth_attachment.format = m_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		VkAttachmentReference depth_reference = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		// subpass
		VkSubpassDescription subpass_desc{};
		subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_desc.pDepthStencilAttachment = &depth_reference;

		// subpass dependencies
		std::vector<VkSubpassDependency> dependencies = 
		{
			{
				VK_SUBPASS_EXTERNAL,
				0,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				0
			},
			{
				0,
				VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				0
			}
		};

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = 1;
		render_pass_ci.pAttachments = &depth_attachment;
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass_desc;
		render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
		render_pass_ci.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create render pass");
	}

	void DirectionalLightShadowPass::createDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{12, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
		};

		VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
		desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());

		m_desc_set_layouts.resize(2);
		VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create static mesh descriptor set layout");

		desc_set_layout_bindings.insert(desc_set_layout_bindings.begin(), { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data(); 
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[1]);
		CHECK_VULKAN_RESULT(result, "create skeletal mesh descriptor set layout");
	}

	void DirectionalLightShadowPass::createPipelineLayouts()
	{
		VkPipelineLayoutCreateInfo pipeline_layout_ci{};
		pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_ci.setLayoutCount = 1;
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[0];
		pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
		pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();

		m_pipeline_layouts.resize(2);
		VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create static mesh pipeline layout");

		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[1];
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[1]);
		CHECK_VULKAN_RESULT(result, "create skeletal mesh pipeline layout");
	}

	void DirectionalLightShadowPass::createPipelines()
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
			shader_manager->getShaderStageCI("directional_light_shadow.geom", VK_SHADER_STAGE_GEOMETRY_BIT),
			shader_manager->getShaderStageCI("directional_light_shadow.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// create graphics pipeline
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.pVertexInputState = &vertex_input_ci;
		m_pipeline_ci.layout = m_pipeline_layouts[0];
		m_pipeline_ci.renderPass = m_render_pass;
		m_pipeline_ci.subpass = 0;

		m_pipelines.resize(2);
		VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[0]);
		CHECK_VULKAN_RESULT(result, "create directional light shadow pass's static mesh graphics pipeline");

		// skeletal mesh vertex attributes
		vertex_input_binding_descriptions[0].stride = sizeof(SkeletalVertex);

		vertex_input_attribute_descriptions.resize(5);
		vertex_input_attribute_descriptions[3].binding = 0;
		vertex_input_attribute_descriptions[3].location = 3;
		vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SINT;
		vertex_input_attribute_descriptions[3].offset = offsetof(SkeletalVertex, m_bones);

		vertex_input_attribute_descriptions[4].binding = 0;
		vertex_input_attribute_descriptions[4].location = 4;
		vertex_input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_input_attribute_descriptions[4].offset = offsetof(SkeletalVertex, m_weights);

		vertex_input_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
		vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

		m_pipeline_ci.layout = m_pipeline_layouts[1];
		shader_stage_cis[0] = shader_manager->getShaderStageCI("skeletal_mesh.vert", VK_SHADER_STAGE_VERTEX_BIT);
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[1]);
		CHECK_VULKAN_RESULT(result, "create directional light shadow pass's static mesh graphics pipeline");
	}

	void DirectionalLightShadowPass::createFramebuffer()
	{
		// create depth image view sampler
		VulkanUtil::createImageViewSampler(m_size, m_size, nullptr, 1, SHADOW_CASCADE_NUM, m_format,
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_shadow_image_view_sampler,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		// create framebuffer
		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = 1;
		framebuffer_ci.pAttachments = &m_shadow_image_view_sampler.view;
		framebuffer_ci.width = m_size;
		framebuffer_ci.height = m_size;
		framebuffer_ci.layers = SHADOW_CASCADE_NUM;

		VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
		CHECK_VULKAN_RESULT(result, "create directional light shadow framebuffer");
	}

	void DirectionalLightShadowPass::destroyResizableObjects()
	{
		m_shadow_image_view_sampler.destroy();

		RenderPass::destroyResizableObjects();
	}

	void DirectionalLightShadowPass::updateCascades(const ShadowCascadeCreateInfo& shadow_cascade_ci)
	{
		float cascade_splits[SHADOW_CASCADE_NUM];

		float near = shadow_cascade_ci.camera_near;
		float far = shadow_cascade_ci.camera_far;
		float cascade_frustum_near = shadow_cascade_ci.light_cascade_frustum_near;
		float range = far - near;

		float min_z = near;
		float max_z = near + range;

		float range_z = max_z - min_z;
		float ratio = max_z / min_z;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_CASCADE_NUM; ++i) 
		{
			float p = (i + 1) / static_cast<float>(SHADOW_CASCADE_NUM);
			float log = min_z * std::pow(ratio, p);
			float uniform = min_z + range_z * p;
			float d = m_cascade_split_lambda * (log - uniform) + uniform;
			cascade_splits[i] = (d - near) / range;
		}

		// Calculate orthographic projection matrix for each cascade
		float last_cascade_split = 0.0f;
		for (uint32_t c = 0; c < SHADOW_CASCADE_NUM; ++c)
		{
			float cascade_split = cascade_splits[c];
			glm::vec3 frustum_corners[8] =
			{
				glm::vec3(-1.0f, 1.0f, 0.0f),
				glm::vec3(1.0f, 1.0f, 0.0f),
				glm::vec3(1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, -1.0f, 0.0f),
				glm::vec3(-1.0f, 1.0f, 1.0f),
				glm::vec3(1.0f, 1.0f, 1.0f),
				glm::vec3(1.0f, -1.0f, 1.0f),
				glm::vec3(-1.0f, -1.0f, 1.0f)
			};

			// Project frustum corners into world space
			for (uint32_t i = 0; i < 8; ++i)
			{
				glm::vec4 inv_frustum_corner = shadow_cascade_ci.inv_camera_view_proj * glm::vec4(frustum_corners[i], 1.0f);
				frustum_corners[i] = inv_frustum_corner / inv_frustum_corner.w;
			}

			for (uint32_t i = 0; i < 4; ++i)
			{
				glm::vec3 dist = frustum_corners[i + 4] - frustum_corners[i];
				frustum_corners[i + 4] = frustum_corners[i] + (dist * cascade_split);
				frustum_corners[i] = frustum_corners[i] + (dist * last_cascade_split);
			}

			// Get frustum center
			glm::vec3 frustum_center = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; ++i) 
			{
				frustum_center += frustum_corners[i];
			}
			frustum_center /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; ++i) 
			{
				float distance = glm::length(frustum_corners[i] - frustum_center);
				radius = std::max(radius, distance);
			}

			glm::mat4 light_view = glm::lookAtRH(frustum_center - shadow_cascade_ci.light_dir * radius, frustum_center, k_up_vector);
			glm::mat4 light_proj = glm::orthoRH_ZO(-radius, radius, -radius, radius, cascade_frustum_near, radius * 2.0f);
			light_proj[1][1] *= -1.0f;

			// Store split distance and matrix in cascade
			m_shadow_cascade_ubo.cascade_view_projs[c] = light_proj * light_view;
			m_cascade_splits[c] = -(near + cascade_split * range);

			last_cascade_split = cascade_split;
		}

		// update uniform buffers
		VulkanUtil::updateBuffer(m_shadow_cascade_ubs, (void*)&m_shadow_cascade_ubo, sizeof(ShadowCascadeUBO));
	}

}