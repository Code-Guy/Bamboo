#include "filter_cube_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/resource/shader/shader_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/resource/asset/base/mesh.h"
#include "runtime/resource/asset/texture_cube.h"
#include "runtime/resource/asset/static_mesh.h"
#include "runtime/platform/timer/timer.h"
#include "runtime/platform/string/string_util.h"

#include <array>
#include <fstream>

#define PI 3.1415926f

namespace Bamboo
{

	struct IrradiancePCO
	{
		glm::mat4 mvp;
		float delta_phi;
		float delta_theta;
	};

	struct PrefilterEnvPCO
	{
		glm::mat4 mvp;
		float roughness;
		uint32_t samples;
	};

	FilterCubePass::FilterCubePass()
	{
		m_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		m_formats[1] = VK_FORMAT_R16G16B16A16_SFLOAT;

		m_sizes[0] = 64;
		m_sizes[1] = 512;
		for (int i = 0; i < 2; ++i)
		{
			m_mip_levels[i] = VulkanUtil::calcMipLevel(m_sizes[i]);
		}

		m_skybox_texture_cube = g_runtime_context.assetManager()->loadAsset<TextureCube>("asset/engine/texture/ibl/texc_skybox.texc");
		m_skybox_mesh = g_runtime_context.assetManager()->loadAsset<StaticMesh>("asset/engine/mesh/primitive/sm_box.sm");
	}

	void FilterCubePass::render()
	{
		for (uint32_t i = 0; i < 2; ++i)
		{
			EFilterType FilterType = (EFilterType)i;

			uint32_t width = m_sizes[i];
			uint32_t height = m_sizes[i];

			StopWatch stop_watch;
			stop_watch.start();

			// render to framebuffer
			VkClearValue clear_values[1];
			clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

			VkRenderPassBeginInfo render_pass_bi{};
			render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_bi.renderPass = m_render_passes[i];
			render_pass_bi.renderArea.extent.width = width;
			render_pass_bi.renderArea.extent.height = height;
			render_pass_bi.clearValueCount = 1;
			render_pass_bi.pClearValues = clear_values;
			render_pass_bi.framebuffer = m_framebuffers[i];

			std::vector<glm::mat4> matrices = 
			{
				glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
				glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			};

			VkViewport viewport{};
			viewport.width = static_cast<float>(width);
			viewport.height = static_cast<float>(height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{};
			scissor.extent.width = width;
			scissor.extent.height = height;

			// transition cube image to transfer dst optimal
			VkImage cube_image = m_cube_image_view_samplers[i].image();
			VulkanUtil::transitionImageLayout(cube_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_formats[i], m_mip_levels[i], 6);

			for (uint32_t f = 0; f < 6; ++f)
			{
				for (uint32_t m = 0; m < m_mip_levels[i]; ++m)
				{
					VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();
					vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

					viewport.width = static_cast<float>(m_sizes[i] >> m);
					viewport.height = viewport.width;
					vkCmdSetViewport(command_buffer, 0, 1, &viewport);
					vkCmdSetScissor(command_buffer, 0, 1, &scissor);

					glm::mat4 mvp = glm::perspective(PI / 2.0f, 1.0f, 0.1f, 512.0f) * matrices[f];
					switch (FilterType)
					{
					case EFilterType::Irradiance:
					{
						IrradiancePCO irradiance_pco;
						irradiance_pco.mvp = mvp;
						irradiance_pco.delta_phi = PI / 90.0f;
						irradiance_pco.delta_theta = PI / 128.0f;
						vkCmdPushConstants(command_buffer, m_pipeline_layouts[i], 
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
							0, sizeof(IrradiancePCO), &irradiance_pco);
					}
						break;
					case EFilterType::PrefilterEnv:
					{
						PrefilterEnvPCO prefilter_env_pco;
						prefilter_env_pco.mvp = mvp;
						prefilter_env_pco.roughness = (float)m / (float)(m_mip_levels[i] - 1);
						prefilter_env_pco.samples = 32;
						vkCmdPushConstants(command_buffer, m_pipeline_layouts[i], 
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
							0, sizeof(PrefilterEnvPCO), &prefilter_env_pco);
					}
						break;
					default:
						break;
					}

					// update(push) descriptors
					VkWriteDescriptorSet desc_write{};

					VkDescriptorImageInfo desc_image_info{};
					desc_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					desc_image_info.imageView = m_skybox_texture_cube->m_image_view_sampler.view;
					desc_image_info.sampler = m_skybox_texture_cube->m_image_view_sampler.sampler;

					desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					desc_write.dstSet = 0;
					desc_write.dstBinding = 0;
					desc_write.dstArrayElement = 0;
					desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					desc_write.descriptorCount = 1;
					desc_write.pImageInfo = &desc_image_info;

					VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						m_pipeline_layouts[i], 0, 1, &desc_write);

					// bind pipeline
					vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[i]);

					// bind vertex and index buffer
					VkBuffer vertexBuffers[] = { m_skybox_mesh->m_vertex_buffer.buffer};
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
					vkCmdBindIndexBuffer(command_buffer, m_skybox_mesh->m_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

					// draw indexed mesh
					vkCmdDrawIndexed(command_buffer, m_skybox_mesh->m_sub_meshes[0].m_index_count, 1, m_skybox_mesh->m_sub_meshes[0].m_index_offset, 0, 0);

					vkCmdEndRenderPass(command_buffer);

					VulkanUtil::endInstantCommands(command_buffer);

					// transition framebuffer texture to transfer src optimal
					VkImage color_image = m_color_image_views[i].image();
					VulkanUtil::transitionImageLayout(color_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

					// copy framebuffer texture to cube map texture
					VkImageCopy image_copy{};
					image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					image_copy.srcSubresource.baseArrayLayer = 0;
					image_copy.srcSubresource.mipLevel = 0;
					image_copy.srcSubresource.layerCount = 1;
					image_copy.srcOffset = { 0, 0, 0 };

					image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					image_copy.dstSubresource.baseArrayLayer = f;
					image_copy.dstSubresource.mipLevel = m;
					image_copy.dstSubresource.layerCount = 1;
					image_copy.dstOffset = { 0, 0, 0 };

					image_copy.extent.width = static_cast<uint32_t>(viewport.width);
					image_copy.extent.height = static_cast<uint32_t>(viewport.height);
					image_copy.extent.depth = 1;

					command_buffer = VulkanUtil::beginInstantCommands();
					vkCmdCopyImage(
						command_buffer,
						color_image,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						cube_image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1,
						&image_copy);
					VulkanUtil::endInstantCommands(command_buffer);

					// save framebuffer texture data to file
// 					VulkanUtil::saveImage(color_image, static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height), 
// 						m_formats[i], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, StringUtil::format("D:/Test/filter_cube_%d_%d_%d.bin", i, f, m));

					// transition framebuffer texture to color attachment optimal
					VulkanUtil::transitionImageLayout(color_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				}
			}

			// transition cube image to read only optimal
			VulkanUtil::transitionImageLayout(cube_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_formats[i], m_mip_levels[i], 6);
		}
	}

	void FilterCubePass::destroy()
	{
		RenderPass::destroy();

		for (int i = 0; i < 2; ++i)
		{
			vkDestroyRenderPass(VulkanRHI::get().getDevice(), m_render_passes[i], nullptr);
		}
	}

	void FilterCubePass::createRenderPass()
	{
		for (uint32_t i = 0; i < 2; ++i)
		{
			EFilterType FilterType = (EFilterType)i;

			// color attachment
			VkAttachmentDescription color_attachment{};
			color_attachment.format = m_formats[i];
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkAttachmentReference color_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			// subpass
			VkSubpassDescription subpass_desc{};
			subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_desc.colorAttachmentCount = 1;
			subpass_desc.pColorAttachments = &color_reference;

			// subpass dependencies
			std::array<VkSubpassDependency, 2> dependencies;
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// create render pass
			VkRenderPassCreateInfo render_pass_ci{};
			render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			render_pass_ci.attachmentCount = 1;
			render_pass_ci.pAttachments = &color_attachment;
			render_pass_ci.subpassCount = 1;
			render_pass_ci.pSubpasses = &subpass_desc;
			render_pass_ci.dependencyCount = 2;
			render_pass_ci.pDependencies = dependencies.data();

			VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_passes[i]);
			CHECK_VULKAN_RESULT(result, "create render pass");
		}
	}

	void FilterCubePass::createDescriptorSetLayouts()
	{
		m_desc_set_layouts.resize(2);

		for (uint32_t i = 0; i < 2; ++i)
		{
			VkDescriptorSetLayoutBinding desc_set_layout_bindings = {
				0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
			};

			VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
			desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			desc_set_layout_ci.pBindings = &desc_set_layout_bindings;
			desc_set_layout_ci.bindingCount = 1;

			VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[i]);
			CHECK_VULKAN_RESULT(result, "create descriptor set layout");
		}
	}

	void FilterCubePass::createPipelineLayouts()
	{
		m_pipeline_layouts.resize(2);

		for (uint32_t i = 0; i < 2; ++i)
		{
			EFilterType FilterType = (EFilterType)i;

			VkPushConstantRange push_constant_range{};
			push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			switch (FilterType)
			{
			case EFilterType::Irradiance:
				push_constant_range.size = sizeof(IrradiancePCO);
				break;
			case EFilterType::PrefilterEnv:
				push_constant_range.size = sizeof(PrefilterEnvPCO);
				break;
			default:
				break;
			}

			VkPipelineLayoutCreateInfo pipeline_layout_ci{};
			pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeline_layout_ci.setLayoutCount = 1;
			pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[i];
			pipeline_layout_ci.pushConstantRangeCount = 1;
			pipeline_layout_ci.pPushConstantRanges = &push_constant_range;

			VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[i]);
			CHECK_VULKAN_RESULT(result, "create pipeline layout");
		}
	}

	void FilterCubePass::createPipelines()
	{
		m_pipelines.resize(2);

		for (uint32_t i = 0; i < 2; ++i)
		{
			EFilterType FilterType = (EFilterType)i;

			// disable culling and depth testing
			m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
			m_depth_stencil_ci.depthTestEnable = VK_FALSE;
			m_depth_stencil_ci.depthWriteEnable = VK_FALSE;

			// vertex input state
			// vertex bindings
			std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions;
			vertex_input_binding_descriptions.resize(1, VkVertexInputBindingDescription{});
			vertex_input_binding_descriptions[0].binding = 0;
			vertex_input_binding_descriptions[0].stride = sizeof(StaticVertex);
			vertex_input_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			// vertex attributes
			std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions;
			vertex_input_attribute_descriptions.resize(1, VkVertexInputAttributeDescription{});

			vertex_input_attribute_descriptions[0].binding = 0;
			vertex_input_attribute_descriptions[0].location = 0;
			vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertex_input_attribute_descriptions[0].offset = offsetof(StaticVertex, m_position);

			VkPipelineVertexInputStateCreateInfo vertex_input_ci{};
			vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertex_input_ci.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_binding_descriptions.size());
			vertex_input_ci.pVertexBindingDescriptions = vertex_input_binding_descriptions.data();
			vertex_input_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
			vertex_input_ci.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

			// shader stages
			const auto& shader_manager = g_runtime_context.shaderManager();
			std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
				shader_manager->getShaderStageCI("filter_cube.vert", VK_SHADER_STAGE_VERTEX_BIT),
				shader_manager->getShaderStageCI(
					FilterType == EFilterType::Irradiance ? "irradiance.frag" : "prefilter_env.frag",
					VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			// create graphics pipeline
			m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
			m_pipeline_ci.pStages = shader_stage_cis.data();
			m_pipeline_ci.pVertexInputState = &vertex_input_ci;
			m_pipeline_ci.layout = m_pipeline_layouts[i];
			m_pipeline_ci.renderPass = m_render_passes[i];
			m_pipeline_ci.subpass = 0;

			VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[i]);
			CHECK_VULKAN_RESULT(result, "create brdf lut graphics pipeline");
		}
	}

	void FilterCubePass::createFramebuffer()
	{
		for (uint32_t i = 0; i < 2; ++i)
		{
			EFilterType FilterType = (EFilterType)i;

			uint32_t width = m_sizes[i];
			uint32_t height = m_sizes[i];

			// 1.create color images and views
			VulkanUtil::createImageAndView(width, height, 1, 1, VK_SAMPLE_COUNT_1_BIT, m_formats[i],
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
				VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
				VK_IMAGE_ASPECT_COLOR_BIT, m_color_image_views[i]);
			
			// 2.create cubemap images and views
			VulkanUtil::createImageViewSampler(width, height, nullptr, m_mip_levels[i], 6, m_formats[i],
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_cube_image_view_samplers[i]);

			// 3.create framebuffer
			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = m_render_passes[i];
			framebuffer_ci.attachmentCount = 1;
			framebuffer_ci.pAttachments = &m_color_image_views[i].view;
			framebuffer_ci.width = width;
			framebuffer_ci.height = height;
			framebuffer_ci.layers = 1;

			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffers[i]);
			CHECK_VULKAN_RESULT(result, "create brdf lut graphics pipeline");
		}
	}

	void FilterCubePass::destroyResizableObjects()
	{
		for (int i = 0; i < 2; ++i)
		{
			m_color_image_views[i].destroy();
			m_cube_image_view_samplers[i].destroy();
		}

		for (int i = 0; i < 2; ++i)
		{
			vkDestroyFramebuffer(VulkanRHI::get().getDevice(), m_framebuffers[i], nullptr);
		}

		RenderPass::destroyResizableObjects();
	}

}