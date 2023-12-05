#include "spot_light_shadow_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/resource/shader/shader_manager.h"
#include "engine/resource/asset/base/mesh.h"
#include "engine/core/math/transform.h"

namespace Bamboo
{

	SpotLightShadowPass::SpotLightShadowPass()
	{
		m_format = VulkanRHI::get().getDepthFormat();
		m_size = 1024;
		m_light_num = 0;
	}

	void SpotLightShadowPass::init()
	{
		RenderPass::init();

		createResizableObjects(m_size, m_size);
		createDynamicBuffers(MAX_SPOT_LIGHT_NUM);
	}

	void SpotLightShadowPass::render()
	{
		for (size_t p = 0; p < m_light_num; ++p)
		{
			VkRenderPassBeginInfo render_pass_bi{};
			render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_bi.renderPass = m_render_pass;
			render_pass_bi.framebuffer = m_framebuffers[p];
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
				for (size_t i = 0; i < sub_mesh_count; ++i)
				{
					// push constants
					TransformPCO transform_pco = static_mesh_render_data->transform_pco;
					transform_pco.mvp = m_light_view_projs[p] * transform_pco.m;
					updatePushConstants(command_buffer, pipeline_layout, { &transform_pco });

					// update(push) sub mesh descriptors
					std::vector<VkWriteDescriptorSet> desc_writes;
					std::array<VkDescriptorBufferInfo, 1> desc_buffer_infos{};
					std::array<VkDescriptorImageInfo, 1> desc_image_infos{};

					// bone matrix ubo
					if (is_skeletal_mesh)
					{
						addBufferDescriptorSet(desc_writes, desc_buffer_infos[0], skeletal_mesh_render_data->bone_ub, 0);
					}

					// base color texture image sampler
					addImageDescriptorSet(desc_writes, desc_image_infos[0], static_mesh_render_data->pbr_textures[i].base_color_texure, 1);

					VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline_layout, 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());

					// render sub mesh
					vkCmdDrawIndexed(command_buffer, index_counts[i], 1, index_offsets[i], 0, 0);
				}
			}

			vkCmdEndRenderPass(command_buffer);
		}

		m_render_datas.clear();
	}

	void SpotLightShadowPass::createRenderPass()
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
			//{
			//	VK_SUBPASS_EXTERNAL,
			//	0,
			//	VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			//	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			//	VK_ACCESS_SHADER_READ_BIT,
			//	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			//	0
			//},
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

	void SpotLightShadowPass::createDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
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

	void SpotLightShadowPass::createPipelineLayouts()
	{
		m_push_constant_ranges =
		{
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TransformPCO) },
		};

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

	void SpotLightShadowPass::createPipelines()
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
			shader_manager->getShaderStageCI("spot_light_shadow.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
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
		CHECK_VULKAN_RESULT(result, "create spot light shadow pass's static mesh graphics pipeline");

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
		CHECK_VULKAN_RESULT(result, "create spot light shadow pass's static mesh graphics pipeline");
	}

	void SpotLightShadowPass::destroyResizableObjects()
	{
		for (auto& shadow_image_view_sampler : m_shadow_image_view_samplers)
		{
			shadow_image_view_sampler.destroy();
		}
		for (auto& framebuffer : m_framebuffers)
		{
			vkDestroyFramebuffer(VulkanRHI::get().getDevice(), framebuffer, nullptr);
		}

		RenderPass::destroyResizableObjects();
	}

	void SpotLightShadowPass::updateFrustums(const std::vector<ShadowFrustumCreateInfo>& shadow_frustum_cis)
	{
		m_light_num = shadow_frustum_cis.size();
		for (size_t p = 0; p < m_light_num; ++p)
		{
			const ShadowFrustumCreateInfo& shadow_frustum_ci = shadow_frustum_cis[p];
			glm::mat4 view = glm::lookAtRH(shadow_frustum_ci.light_pos, shadow_frustum_ci.light_pos + shadow_frustum_ci.light_dir, k_up_vector);
			float fov_angle = std::min(shadow_frustum_ci.light_angle * 2.0f, 180.0f);
			glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(fov_angle), 1.0f, shadow_frustum_ci.light_near, shadow_frustum_ci.light_far);
			proj[1][1] *= -1.0f;
			m_light_view_projs[p] = proj * view;
		}
	}

	const std::vector<VmaImageViewSampler>& SpotLightShadowPass::getShadowImageViewSamplers()
	{
		return m_shadow_image_view_samplers;
	}

	void SpotLightShadowPass::createDynamicBuffers(size_t size)
	{
		size_t last_size = m_framebuffers.size();
		m_shadow_image_view_samplers.resize(size);
		m_framebuffers.resize(size);
		m_light_view_projs.resize(size);

		for (uint32_t i = last_size; i < size; ++i)
		{
			// create shadow image view sampler
			VulkanUtil::createImageViewSampler(m_size, m_size, nullptr, 1, 1, m_format,
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_shadow_image_view_samplers[i],
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

			// create framebuffers
			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = m_render_pass;
			framebuffer_ci.attachmentCount = 1;
			framebuffer_ci.pAttachments = &m_shadow_image_view_samplers[i].view;
			framebuffer_ci.width = m_size;
			framebuffer_ci.height = m_size;
			framebuffer_ci.layers = 1;

			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffers[i]);
			CHECK_VULKAN_RESULT(result, "create spot light shadow framebuffer");
		}
	}

}