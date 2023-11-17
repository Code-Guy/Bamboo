#include "main_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/resource/shader/shader_manager.h"
#include "engine/resource/asset/asset_manager.h"
#include "engine/function/render/debug_draw_manager.h"
#include "engine/platform/timer/timer.h"
#include "engine/resource/asset/base/mesh.h"
#include "engine/function/render/render_data.h"

namespace Bamboo
{

	MainPass::MainPass()
	{
		m_formats = {
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
			VulkanRHI::get().getDepthFormat()
		};
	}

	void MainPass::render()
	{
		VkRenderPassBeginInfo render_pass_bi{};
		render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_bi.renderPass = m_render_pass;
		render_pass_bi.framebuffer = m_framebuffer;
		render_pass_bi.renderArea.offset = { 0, 0 };
		render_pass_bi.renderArea.extent = { m_width, m_height };

		std::array<VkClearValue, 6> clear_values{};
		clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clear_values[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clear_values[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clear_values[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clear_values[5].depthStencil = { 1.0f, 0 };
		render_pass_bi.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_bi.pClearValues = clear_values.data();

		VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();
		uint32_t flight_index = VulkanRHI::get().getFlightIndex();
		vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_width);
		viewport.height = static_cast<float>(m_height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_width, m_height };
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		// 1.deferred subpass
		for (const auto& render_data : m_render_datas)
		{
			render_mesh(render_data, ERendererType::Deferred);
		}

		// 2.composition subpass
		vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

		if (!m_render_datas.empty())
		{
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[2]);

			std::vector<VkWriteDescriptorSet> desc_writes;
			std::array<VkDescriptorBufferInfo, 1> desc_buffer_infos{};

			// lighting uniform buffer
			addBufferDescriptorSet(desc_writes, desc_buffer_infos[0], m_lighting_render_data->lighting_ubs[flight_index], 11);

			// input attachments and ibl textures
			std::vector<VmaImageViewSampler> textures = {
				m_normal_texture_sampler,
				m_base_color_texture_sampler,
				m_emissive_texture_sampler,
				m_metallic_roughness_occlusion_texture_sampler,
				m_depth_stencil_texture_sampler,
				m_lighting_render_data->irradiance_texture,
				m_lighting_render_data->prefilter_texture,
				m_lighting_render_data->brdf_lut_texture,
				m_lighting_render_data->directional_light_shadow_texture
			};
			std::vector<VmaImageViewSampler> point_light_shadow_textures = m_lighting_render_data->point_light_shadow_textures;
			std::vector<VmaImageViewSampler> spot_light_shadow_textures = m_lighting_render_data->spot_light_shadow_textures;
			std::vector<VkDescriptorImageInfo> desc_image_infos(textures.size() + point_light_shadow_textures.size() + spot_light_shadow_textures.size(), VkDescriptorImageInfo{});
			for (size_t i = 0; i < textures.size(); ++i)
			{
				addImageDescriptorSet(desc_writes, desc_image_infos[i], textures[i], i);
			}
			addImagesDescriptorSet(desc_writes, &desc_image_infos[textures.size()], point_light_shadow_textures, textures.size());
			addImagesDescriptorSet(desc_writes, &desc_image_infos[textures.size() + point_light_shadow_textures.size()], spot_light_shadow_textures, textures.size() + 1);

			VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_pipeline_layouts[2], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
			vkCmdDraw(command_buffer, 3, 1, 0, 0);
		}

		// 3.forward subpass
		vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

		// 3.1 debug draw
		const auto& ddm = g_engine.debugDrawSystem();
		if (!ddm->empty())
		{
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[6]);

			// bind vertex and index buffer
			VkBuffer vertexBuffers[] = { ddm->getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

			// push constants
			vkCmdPushConstants(command_buffer, m_pipeline_layouts[6], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &m_lighting_render_data->camera_view_proj);
			vkCmdDraw(command_buffer, ddm->getVertexCount(), 1, 0, 0);
		}

		// 3.2 render skybox
		if (m_skybox_render_data)
		{
			// bind pipeline
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[5]);

			// bind vertex and index buffer
			VkBuffer vertexBuffers[] = { m_skybox_render_data->vertex_buffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(command_buffer, m_skybox_render_data->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			// push constants
			vkCmdPushConstants(command_buffer, m_pipeline_layouts[5], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TransformPCO), &m_skybox_render_data->transform_pco);

			// update(push) sub mesh descriptors
			std::vector<VkWriteDescriptorSet> desc_writes;
			VkDescriptorImageInfo desc_image_info{};
			addImageDescriptorSet(desc_writes, desc_image_info, m_skybox_render_data->env_texture, 0);

			VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_pipeline_layouts[5], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
			vkCmdDrawIndexed(command_buffer, m_skybox_render_data->index_count, 1, 0, 0, 0);
		}

		// 3.3 render transparency meshes
		for (const auto& render_data : m_transparency_render_datas)
		{
			render_mesh(render_data, ERendererType::Forward);
		}

		// 3.4 render billboards
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[7]);
		for (const auto& render_data : m_billboard_render_datas)
		{
			// push constants
			vkCmdPushConstants(command_buffer, m_pipeline_layouts[7], VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT,
				0, sizeof(glm::vec4) + sizeof(glm::vec2), &render_data->position);

			std::vector<VkWriteDescriptorSet> desc_writes;
			VkDescriptorImageInfo desc_image_info{};
			addImageDescriptorSet(desc_writes, desc_image_info, render_data->texture, 0);

			VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_pipeline_layouts[7], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
			vkCmdDraw(command_buffer, 1, 1, 0, 0);
		}

		vkCmdEndRenderPass(command_buffer);
	}

	void MainPass::createRenderPass()
	{
		// attachments
		std::array<VkAttachmentDescription, 6> attachments{};
		std::array<VkAttachmentReference, 6> references{};
		std::array<VkAttachmentReference, 5> input_references{};
		for (uint32_t i = 0; i < 6; ++i)
		{
			attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[i].storeOp = (i == 0 || i == 5) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[i].finalLayout = i == 0 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : (
				i == 5 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			attachments[i].format = m_formats[i];

			references[i].attachment = i;
			references[i].layout = i == 5 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		for (uint32_t i = 0; i < 5; ++i)
		{
			input_references[i].attachment = i + 1;
			input_references[i].layout = i == 4 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		// subpasses
		std::array<VkSubpassDescription, 3> subpass_descs{};

		// gbuffer subpass
		subpass_descs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_descs[0].colorAttachmentCount = 4;
		subpass_descs[0].pColorAttachments = &references[1];
		subpass_descs[0].pDepthStencilAttachment = &references[5];

		// composition subpass
		subpass_descs[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_descs[1].colorAttachmentCount = 1;
		subpass_descs[1].pColorAttachments = &references[0];
		subpass_descs[1].inputAttachmentCount = 5;
		subpass_descs[1].pInputAttachments = &input_references[0];

		// forward subpass
		subpass_descs[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_descs[2].colorAttachmentCount = 1;
		subpass_descs[2].pColorAttachments = &references[0];
		subpass_descs[2].pDepthStencilAttachment = &references[5];

		// subpass dependencies
		std::array<VkSubpassDependency, 8> dependencies{};

		// transitions the gbuffer input attachment from color attachment to shader read
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 1;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dependencyFlags = 0;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 1;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		dependencies[2].srcSubpass = 1;
		dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dependencyFlags = 0;

		dependencies[3].srcSubpass = 1;
		dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[3].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[3].dependencyFlags = 0;

		dependencies[4].srcSubpass = 0;
		dependencies[4].dstSubpass = 1;
		dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[4].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[4].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[5].srcSubpass = 0;
		dependencies[5].dstSubpass = 1;
		dependencies[5].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[5].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[5].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[5].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[5].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[6].srcSubpass = 1;
		dependencies[6].dstSubpass = 2;
		dependencies[6].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[6].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[6].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[6].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[6].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[7].srcSubpass = 2;
		dependencies[7].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[7].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[7].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[7].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[7].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[7].dependencyFlags = 0;

		// create render pass
		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_ci.pAttachments = attachments.data();
		render_pass_ci.subpassCount = static_cast<uint32_t>(subpass_descs.size());
		render_pass_ci.pSubpasses = subpass_descs.data();
		render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
		render_pass_ci.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create render pass");
	}

	void MainPass::createDescriptorSetLayouts()
	{
		// gbuffer descriptor set layouts
		std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
		desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();

		m_desc_set_layouts.resize(8);
		VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create gbuffer static mesh descriptor set layout");

		desc_set_layout_bindings.insert(desc_set_layout_bindings.begin(), { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[1]);
		CHECK_VULKAN_RESULT(result, "create gbuffer skeletal mesh descriptor set layout");

		// composition descriptor set layouts
		desc_set_layout_bindings = {
			{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_POINT_LIGHT_NUM, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SPOT_LIGHT_NUM, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{11, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[2]);
		CHECK_VULKAN_RESULT(result, "create composition descriptor set layout");

		// transparency descriptor set layouts
		desc_set_layout_bindings = {
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_POINT_LIGHT_NUM, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SPOT_LIGHT_NUM, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{11, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		};

		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[3]);
		CHECK_VULKAN_RESULT(result, "create transparency static mesh descriptor set layout");

		desc_set_layout_bindings.insert(desc_set_layout_bindings.begin(), { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr });
		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[4]);
		CHECK_VULKAN_RESULT(result, "create transparency skeletal mesh descriptor set layout");

		// skybox descriptor set layouts
		desc_set_layout_bindings = {
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
		};

		desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
		desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[5]);
		CHECK_VULKAN_RESULT(result, "create skybox descriptor set layout");

		// billboard descriptor set layouts
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[7]);
		CHECK_VULKAN_RESULT(result, "create billboard descriptor set layout");

		// debug draw descriptor set layouts
		desc_set_layout_ci.bindingCount = 0;
		desc_set_layout_ci.pBindings = nullptr;
		result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[6]);
		CHECK_VULKAN_RESULT(result, "create debug draw descriptor set layout");
	}

	void MainPass::createPipelineLayouts()
	{
		// gbuffer pipeline layouts
		VkPipelineLayoutCreateInfo pipeline_layout_ci{};
		pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_ci.setLayoutCount = 1;
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[0];

		m_push_constant_ranges =
		{
			{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TransformPCO) },
			{ VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(TransformPCO), sizeof(MaterialPCO) }
		};

		pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
		pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();

		m_pipeline_layouts.resize(8);
		VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[0]);
		CHECK_VULKAN_RESULT(result, "create gbuffer static mesh pipeline layout");

		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[1];
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[1]);
		CHECK_VULKAN_RESULT(result, "create gbuffer skeletal mesh pipeline layout");

		// composition pipeline layouts
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[2];
		pipeline_layout_ci.pushConstantRangeCount = 0;
		pipeline_layout_ci.pPushConstantRanges = nullptr;
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[2]);
		CHECK_VULKAN_RESULT(result, "create composition pipeline layout");

		// transparency pipeline layouts
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[3];
		pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
		pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[3]);
		CHECK_VULKAN_RESULT(result, "create transparency static mesh pipeline layout");

		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[4];
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[4]);
		CHECK_VULKAN_RESULT(result, "create transparency skeletal mesh pipeline layout");

		// skybox pipeline layouts
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[5];
		pipeline_layout_ci.pushConstantRangeCount = 1;
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[5]);
		CHECK_VULKAN_RESULT(result, "create skybox pipeline layout");

		// debug draw pipeline layouts
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[6];
		VkPushConstantRange push_constant_range = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) };
		pipeline_layout_ci.pPushConstantRanges = &push_constant_range;
		pipeline_layout_ci.pushConstantRangeCount = 1;
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[6]);
		CHECK_VULKAN_RESULT(result, "create debug draw pipeline layout");

		// billboard pipeline layouts
		pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[7];
		push_constant_range = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(glm::vec4) + sizeof(glm::vec2) };
		result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[7]);
		CHECK_VULKAN_RESULT(result, "create billboard pipeline layout");
	}

	void MainPass::createPipelines()
	{
		// color blending
		for (int i = 0; i < 3; ++i)
		{
			m_color_blend_attachments.push_back(m_color_blend_attachments.front());
		}
		m_color_blend_ci.attachmentCount = static_cast<uint32_t>(m_color_blend_attachments.size());
		m_color_blend_ci.pAttachments = m_color_blend_attachments.data();

		// vertex input
		// vertex bindings
		std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions;
		vertex_input_binding_descriptions.resize(1, VkVertexInputBindingDescription{});
		vertex_input_binding_descriptions[0].binding = 0;
		vertex_input_binding_descriptions[0].stride = sizeof(StaticVertex);
		vertex_input_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// vertex attributes
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
			shader_manager->getShaderStageCI("gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// create graphics pipeline
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.pVertexInputState = &vertex_input_ci;
		m_pipeline_ci.layout = m_pipeline_layouts[0];
		m_pipeline_ci.renderPass = m_render_pass;
		m_pipeline_ci.subpass = 0;

		m_pipelines.resize(8);

		// create gbuffer static mesh pipeline
		VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[0]);
		CHECK_VULKAN_RESULT(result, "create gbuffer static mesh graphics pipeline");

		// create transparency static mesh pipeline
		m_color_blend_ci.attachmentCount = 1;
		m_color_blend_attachments[0].blendEnable = VK_TRUE;
		shader_stage_cis[1] = shader_manager->getShaderStageCI("forward_lighting.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
		m_pipeline_ci.layout = m_pipeline_layouts[3];
		m_pipeline_ci.subpass = 2;
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[3]);
		CHECK_VULKAN_RESULT(result, "create transparency static mesh graphics pipeline");

		// skybox pipeline
		shader_stage_cis = {
			shader_manager->getShaderStageCI("skybox.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("skybox.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		m_color_blend_attachments[0].blendEnable = VK_FALSE;
		m_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
		m_pipeline_ci.layout = m_pipeline_layouts[5];
		m_pipeline_ci.subpass = 2;
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[5]);
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
		m_color_blend_ci.attachmentCount = static_cast<uint32_t>(m_color_blend_attachments.size());
		shader_stage_cis[1] = shader_manager->getShaderStageCI("gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
		CHECK_VULKAN_RESULT(result, "create skybox graphics pipeline");
		m_depth_stencil_ci.depthCompareOp = VK_COMPARE_OP_LESS;

		// create gbuffer skeletal mesh pipeline
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
		m_pipeline_ci.subpass = 0;
		shader_stage_cis[0] = shader_manager->getShaderStageCI("skeletal_mesh.vert", VK_SHADER_STAGE_VERTEX_BIT);
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[1]);
		CHECK_VULKAN_RESULT(result, "create gbuffer skeletal mesh graphics pipeline");

		// create transparency skeletal mesh pipeline
		m_color_blend_ci.attachmentCount = 1;
		m_color_blend_attachments[0].blendEnable = VK_TRUE;
		shader_stage_cis[1] = shader_manager->getShaderStageCI("forward_lighting.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
		m_pipeline_ci.layout = m_pipeline_layouts[4];
		m_pipeline_ci.subpass = 2;
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[4]);
		CHECK_VULKAN_RESULT(result, "create transparency skeletal mesh graphics pipeline");

		// composition pipelines
		// disable culling and depth testing
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
		m_depth_stencil_ci.depthTestEnable = VK_FALSE;
		m_depth_stencil_ci.depthWriteEnable = VK_FALSE;
		m_color_blend_attachments[0].blendEnable = VK_FALSE;

		// vertex input state
		VkPipelineVertexInputStateCreateInfo composition_vertex_input_ci{};
		composition_vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// shader stages
		shader_stage_cis = {
			shader_manager->getShaderStageCI("screen.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("deferred_lighting.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		m_pipeline_ci.pVertexInputState = &composition_vertex_input_ci;
		m_pipeline_ci.layout = m_pipeline_layouts[2];
		m_pipeline_ci.subpass = 1;
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[2]);
		CHECK_VULKAN_RESULT(result, "create composition graphics pipeline");

		// billboard pipeline
		m_input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		m_depth_stencil_ci.depthTestEnable = VK_TRUE;
		m_depth_stencil_ci.depthWriteEnable = VK_TRUE;
		m_color_blend_attachments[0].blendEnable = VK_TRUE;

		// shader stages
		shader_stage_cis = {
			shader_manager->getShaderStageCI("billboard.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("billboard.geom", VK_SHADER_STAGE_GEOMETRY_BIT),
			shader_manager->getShaderStageCI("billboard.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.layout = m_pipeline_layouts[7];
		m_pipeline_ci.subpass = 2;
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[7]);
		CHECK_VULKAN_RESULT(result, "create billboard graphics pipeline");

		// debug draw pipeline
		m_input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		m_color_blend_attachments[0].blendEnable = VK_FALSE;

		// vertex input
		vertex_input_binding_descriptions[0].stride = sizeof(DebugDrawVertex);
		vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_input_ci.vertexAttributeDescriptionCount = 2;

		// shader stages
		shader_stage_cis = {
			shader_manager->getShaderStageCI("debug_draw.vert", VK_SHADER_STAGE_VERTEX_BIT),
			shader_manager->getShaderStageCI("debug_draw.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		m_pipeline_ci.pVertexInputState = &vertex_input_ci;
		m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
		m_pipeline_ci.pStages = shader_stage_cis.data();
		m_pipeline_ci.layout = m_pipeline_layouts[6];
		m_pipeline_ci.subpass = 2;
		result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[6]);
		CHECK_VULKAN_RESULT(result, "create debug draw graphics pipeline");
	}

	void MainPass::createFramebuffer()
	{
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_formats[0],
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_color_texture_sampler,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_formats[1],
			VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_normal_texture_sampler,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_formats[2],
			VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_base_color_texture_sampler,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_formats[3],
			VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_emissive_texture_sampler,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_formats[4],
			VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_metallic_roughness_occlusion_texture_sampler,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_formats[5],
			VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_depth_stencil_texture_sampler,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

		// 2.create framebuffer
		std::vector<VkImageView> attachments = {
			m_color_texture_sampler.view,
			m_normal_texture_sampler.view,
			m_base_color_texture_sampler.view,
			m_emissive_texture_sampler.view,
			m_metallic_roughness_occlusion_texture_sampler.view,
			m_depth_stencil_texture_sampler.view
		};

		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_ci.pAttachments = attachments.data();
		framebuffer_ci.width = m_width;
		framebuffer_ci.height = m_height;
		framebuffer_ci.layers = 1;

		VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
		CHECK_VULKAN_RESULT(result, "create main frame buffer");
	}

	void MainPass::destroyResizableObjects()
	{
		m_color_texture_sampler.destroy();
		m_normal_texture_sampler.destroy();
		m_base_color_texture_sampler.destroy();
		m_emissive_texture_sampler.destroy();
		m_metallic_roughness_occlusion_texture_sampler.destroy();
		m_depth_stencil_texture_sampler.destroy();

		RenderPass::destroyResizableObjects();
	}

	void MainPass::render_mesh(const std::shared_ptr<RenderData>& render_data, ERendererType renderer_type)
	{
		VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();
		uint32_t flight_index = VulkanRHI::get().getFlightIndex();

		std::shared_ptr<SkeletalMeshRenderData> skeletal_mesh_render_data = nullptr;
		std::shared_ptr<StaticMeshRenderData> static_mesh_render_data = std::static_pointer_cast<StaticMeshRenderData>(render_data);
		bool is_skeletal_mesh = render_data->type == ERenderDataType::SkeletalMesh;;
		if (is_skeletal_mesh)
		{
			skeletal_mesh_render_data = std::static_pointer_cast<SkeletalMeshRenderData>(render_data);
		}

		uint32_t pipeline_index = (uint32_t)is_skeletal_mesh + (renderer_type == ERendererType::Deferred ? 0 : 3);
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
			updatePushConstants(command_buffer, pipeline_layout, { &static_mesh_render_data->transform_pco, &static_mesh_render_data->material_pcos[i] });

			// update(push) sub mesh descriptors
			std::vector<VkWriteDescriptorSet> desc_writes;
			std::array<VkDescriptorBufferInfo, 2> desc_buffer_infos{};
			std::array<VkDescriptorImageInfo, 24> desc_image_infos{};

			// bone matrix ubo
			if (is_skeletal_mesh)
			{
				addBufferDescriptorSet(desc_writes, desc_buffer_infos[0], skeletal_mesh_render_data->bone_ubs[flight_index], 0);
			}

			// forward rendering
			if (renderer_type == ERendererType::Forward)
			{
				// lighting ubo
				addBufferDescriptorSet(desc_writes, desc_buffer_infos[1], m_lighting_render_data->lighting_ubs[flight_index], 11);

				// ibl textures
				std::vector<VmaImageViewSampler> ibl_textures = {
					m_lighting_render_data->irradiance_texture,
					m_lighting_render_data->prefilter_texture,
					m_lighting_render_data->brdf_lut_texture,
					m_lighting_render_data->directional_light_shadow_texture,
				};
				const uint32_t k_binding_offset = 5;
				for (size_t t = 0; t < ibl_textures.size(); ++t)
				{
					addImageDescriptorSet(desc_writes, desc_image_infos[t], ibl_textures[t], static_cast<uint32_t>(t + k_binding_offset));
				}

				addImagesDescriptorSet(desc_writes, &desc_image_infos[ibl_textures.size()], m_lighting_render_data->point_light_shadow_textures, ibl_textures.size() + k_binding_offset);
				addImagesDescriptorSet(desc_writes, &desc_image_infos[ibl_textures.size() + m_lighting_render_data->point_light_shadow_textures.size()],
					m_lighting_render_data->spot_light_shadow_textures, ibl_textures.size() + k_binding_offset + 1);
			}

			// image sampler
			std::vector<VmaImageViewSampler> pbr_textures = {
				static_mesh_render_data->pbr_textures[i].base_color_texure,
				static_mesh_render_data->pbr_textures[i].metallic_roughness_occlusion_texure,
				static_mesh_render_data->pbr_textures[i].normal_texure,
				static_mesh_render_data->pbr_textures[i].emissive_texure,
			};
			for (size_t t = 0; t < pbr_textures.size(); ++t)
			{
				addImageDescriptorSet(desc_writes, desc_image_infos[t + 20], pbr_textures[t], static_cast<uint32_t>(t + 1));
			}

			VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline_layout, 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());

			// render sub mesh
			vkCmdDrawIndexed(command_buffer, index_counts[i], 1, index_offsets[i], 0, 0);
		}
	}

}