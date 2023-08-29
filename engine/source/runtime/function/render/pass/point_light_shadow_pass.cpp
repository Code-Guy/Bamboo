#include "point_light_shadow_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/resource/shader/shader_manager.h"
#include "runtime/resource/asset/base/mesh.h"
#include "runtime/resource/asset/texture_cube.h"
#include "runtime/core/math/transform.h"

namespace Bamboo
{

	PointLightShadowPass::PointLightShadowPass()
	{
		m_formats = { VK_FORMAT_R32_SFLOAT, VulkanRHI::get().getDepthFormat() };
		m_size = 1024;
	}

	void PointLightShadowPass::init()
	{
		RenderPass::init();

		createResizableObjects(m_size, m_size);
	}

	void PointLightShadowPass::render()
	{
		for (size_t p = 0; p < m_framebuffers.size(); ++p)
		{
			VkRenderPassBeginInfo render_pass_bi{};
			render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_bi.renderPass = m_render_pass;
			render_pass_bi.framebuffer = m_framebuffers[p];
			render_pass_bi.renderArea.offset = { 0, 0 };
			render_pass_bi.renderArea.extent = { m_size, m_size };

			std::array<VkClearValue, 2> clear_values{};
			clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clear_values[1].depthStencil = { 1.0f, 0 };
			render_pass_bi.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_bi.pClearValues = clear_values.data();

			VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();
			uint32_t flight_index = VulkanRHI::get().getFlightIndex();
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
				EMeshType mesh_type = static_mesh_render_data->mesh_type;
				if (mesh_type == EMeshType::Skeletal)
				{
					skeletal_mesh_render_data = std::static_pointer_cast<SkeletalMeshRenderData>(render_data);
				}

				uint32_t pipeline_index = (uint32_t)mesh_type;
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
					glm::vec4 light_pos = glm::vec4(m_light_poss[p], 1.0f);
					updatePushConstants(command_buffer, pipeline_layout, { &static_mesh_render_data->transform_pco, glm::value_ptr(light_pos) });

					// update(push) sub mesh descriptors
					std::vector<VkWriteDescriptorSet> desc_writes;
					std::array<VkDescriptorBufferInfo, 2> desc_buffer_infos{};
					std::array<VkDescriptorImageInfo, 1> desc_image_infos{};

					// bone matrix ubo
					if (mesh_type == EMeshType::Skeletal)
					{
						addBufferDescriptorSet(desc_writes, desc_buffer_infos[0], skeletal_mesh_render_data->bone_ubs[flight_index], 0);
					}

					// shadow face ubo
					addBufferDescriptorSet(desc_writes, desc_buffer_infos[1], m_shadow_cube_ubss[p][flight_index], 1);

					// base color texture image sampler
					addImageDescriptorSet(desc_writes, desc_image_infos[0], static_mesh_render_data->pbr_textures[i].base_color_texure, 2);

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

	void PointLightShadowPass::destroy()
	{
		RenderPass::destroy();

		// destroy shadow face uniform buffers
		for (auto& shadow_cube_ubs : m_shadow_cube_ubss)
		{
			for (VmaBuffer& uniform_buffer : shadow_cube_ubs)
			{
				uniform_buffer.destroy();
			}
		}
	}

	void PointLightShadowPass::createRenderPass()
	{
		// color attachment
		std::array<VkAttachmentDescription, 2> attachments{};
		attachments[0].format = m_formats[0];
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		// depth attachment
		attachments[1].format = m_formats[1];
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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

		// subpass dependencies
		std::array<VkSubpassDependency, 2> dependencies{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_ci.pAttachments = attachments.data();
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass_desc;
		render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
		render_pass_ci.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create render pass");
	}

	void PointLightShadowPass::createDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
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

	void PointLightShadowPass::createPipelineLayouts()
	{
		m_push_constant_ranges =
		{
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TransformPCO) },
			{ VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(TransformPCO), sizeof(vec4) }
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

	void PointLightShadowPass::createPipelines()
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
		const auto& shader_manager = g_runtime_context.shaderManager();
		std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
			shader_manager->getShaderStageCI("static_mesh.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("point_light_depth.geom", VK_SHADER_STAGE_GEOMETRY_BIT),
			shader_manager->getShaderStageCI("point_light_depth.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
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
		CHECK_VULKAN_RESULT(result, "create position light shadow pass's static mesh graphics pipeline");

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
		CHECK_VULKAN_RESULT(result, "create position light shadow pass's static mesh graphics pipeline");
	}

	void PointLightShadowPass::createFramebuffer()
	{
		// create depth image view sampler
		VulkanUtil::createImageViewSampler(m_size, m_size, nullptr, 1, SHADOW_FACE_NUM, m_formats[1],
			VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_depth_image_view_sampler,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	void PointLightShadowPass::destroyResizableObjects()
	{
		m_depth_image_view_sampler.destroy();
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

	void PointLightShadowPass::updateCubes(const std::vector<ShadowCubeCreateInfo>& shadow_cube_cis)
	{
		createDynamicBuffers(shadow_cube_cis.size());

		for (size_t p = 0; p < shadow_cube_cis.size(); ++p)
		{
			const ShadowCubeCreateInfo& shadow_cube_ci = shadow_cube_cis[p];
			m_light_poss[p] = shadow_cube_ci.light_pos;

			ShadowCubeUBO shadow_cube_ubo;
			glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, shadow_cube_ci.light_near, shadow_cube_ci.light_far);
			for (uint32_t i = 0; i < SHADOW_FACE_NUM; ++i)
			{
				glm::mat4 view = glm::mat4(1.0f);
				switch (i)
				{
				case 0: // POSITIVE_X
					view = glm::rotate(view, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					view = glm::rotate(view, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 1:	// NEGATIVE_X
					view = glm::rotate(view, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					view = glm::rotate(view, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 2:	// POSITIVE_Y
					view = glm::rotate(view, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 3:	// NEGATIVE_Y
					view = glm::rotate(view, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 4:	// POSITIVE_Z
					view = glm::rotate(view, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 5:	// NEGATIVE_Z
					view = glm::rotate(view, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					break;
				}

				shadow_cube_ubo.face_view_projs[i] = proj * view * glm::translate(glm::mat4(1.0f), -m_light_poss[p]);
			}

			// update uniform buffers
			for (VmaBuffer& uniform_buffer : m_shadow_cube_ubss[p])
			{
				VulkanUtil::updateBuffer(uniform_buffer, (void*)&shadow_cube_ubo, sizeof(ShadowCubeUBO));
			}
		}
	}

	const std::vector<Bamboo::VmaImageViewSampler>& PointLightShadowPass::getShadowImageViewSamplers()
	{
		return m_shadow_image_view_samplers;
	}

	void PointLightShadowPass::createDynamicBuffers(size_t size)
	{
		size_t last_size = m_framebuffers.size();
		m_shadow_image_view_samplers.resize(size);
		m_shadow_cube_ubss.resize(size);
		m_framebuffers.resize(size);
		m_light_poss.resize(size);

		for (uint32_t i = last_size; i < size; ++i)
		{
			// create shadow image view sampler
			VulkanUtil::createImageViewSampler(m_size, m_size, nullptr, 1, SHADOW_FACE_NUM, m_formats[0],
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_shadow_image_view_samplers[i],
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

			std::vector<VkImageView> attachments = {
				m_shadow_image_view_samplers[i].view,
				m_depth_image_view_sampler.view
			};

			// create framebuffers
			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = m_render_pass;
			framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_ci.pAttachments = attachments.data();
			framebuffer_ci.width = m_size;
			framebuffer_ci.height = m_size;
			framebuffer_ci.layers = SHADOW_FACE_NUM;

			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffers[i]);
			CHECK_VULKAN_RESULT(result, "create position light shadow framebuffer");

			// create shadow face uniform buffers
			m_shadow_cube_ubss[i].resize(MAX_FRAMES_IN_FLIGHT);
			for (VmaBuffer& uniform_buffer : m_shadow_cube_ubss[i])
			{
				VulkanUtil::createBuffer(sizeof(ShadowCubeUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, uniform_buffer);
			}
		}
	}

}