#include "bloom_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/resource/shader/shader_manager.h"

namespace Bamboo
{
	BloomPass::BloomPass()
	{
		m_format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}

	void BloomPass::init()
	{
		RenderPass::init();

	}

	void BloomPass::render()
	{
		std::shared_ptr<PostProcessRenderData> postprocess_render_data = std::static_pointer_cast<PostProcessRenderData>(m_render_datas.front());
		
		// Brightness
		{
			// render to framebuffer
			std::vector<VkClearValue> clear_values(1);
			clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

			VkRenderPassBeginInfo render_pass_bi{};
			render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_bi.renderPass = m_brightness_pass.render_pass;
			render_pass_bi.renderArea.extent.width = m_width;
			render_pass_bi.renderArea.extent.height = m_height;
			render_pass_bi.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_bi.pClearValues = clear_values.data();
			render_pass_bi.framebuffer = m_brightness_pass.frame_buffer;

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
			{
				vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_brightness_pass.pipelines[0]);

				std::vector<VkWriteDescriptorSet> desc_writes;
				VkDescriptorImageInfo desc_image_info{};

				// push constants
				updatePushConstants(command_buffer, m_brightness_pass.pipeline_layouts[0], { &postprocess_render_data->bloom_fx_data.threshold },
					{ { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) } });

				// texture image samplers
				addImageDescriptorSet(desc_writes, desc_image_info, *postprocess_render_data->p_color_texture, 0);

				VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_brightness_pass.pipeline_layouts[0], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
				vkCmdDraw(command_buffer, 3, 1, 0, 0);
			}
			vkCmdEndRenderPass(command_buffer);
		}

		int blur_direction = 0;
		// Vert Blurred RenderPass
		{
			// render to framebuffer
			std::vector<VkClearValue> clear_values(1);
			clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

			VkRenderPassBeginInfo render_pass_bi{};
			render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_bi.renderPass = m_vert_blurred_pass.render_pass;
			render_pass_bi.renderArea.extent.width = m_width;
			render_pass_bi.renderArea.extent.height = m_height;
			render_pass_bi.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_bi.pClearValues = clear_values.data();
			render_pass_bi.framebuffer = m_vert_blurred_pass.frame_buffer;

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
			{
				vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vert_blurred_pass.pipelines[0]);

				std::vector<VkWriteDescriptorSet> desc_writes;
				VkDescriptorImageInfo desc_image_info{};

				// push constants
				updatePushConstants(command_buffer, m_vert_blurred_pass.pipeline_layouts[0], { &blur_direction },
					{ { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) } });

				// texture image samplers
				addImageDescriptorSet(desc_writes, desc_image_info, m_brightness_pass.brightness_texture_sampler, 0);

				VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_vert_blurred_pass.pipeline_layouts[0], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
				vkCmdDraw(command_buffer, 3, 1, 0, 0);
			}
			vkCmdEndRenderPass(command_buffer);
		}

		blur_direction = 1;
		// Final Bloom RenderPass
		{
			// render to framebuffer
			std::vector<VkClearValue> clear_values(2);
			clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clear_values[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };

			VkRenderPassBeginInfo render_pass_bi{};
			render_pass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_bi.renderPass = m_render_pass;
			render_pass_bi.renderArea.extent.width = m_width;
			render_pass_bi.renderArea.extent.height = m_height;
			render_pass_bi.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_bi.pClearValues = clear_values.data();
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

			// first subpass
			{
				vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0]);

				std::vector<VkWriteDescriptorSet> desc_writes;
				std::array<VkDescriptorImageInfo, 3> desc_image_infos{};

				// push constants
				updatePushConstants(command_buffer, m_pipeline_layouts[0], { &blur_direction },
					{ { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) } });

				// texture image samplers
				addImageDescriptorSet(desc_writes, desc_image_infos[0], m_vert_blurred_pass.vert_blurred_texture_sampler, 0);

				VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_pipeline_layouts[0], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
				vkCmdDraw(command_buffer, 3, 1, 0, 0);
			}

			//second subpass
			{
				vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[1]);

				// push constants
				updatePushConstants(command_buffer, m_pipeline_layouts[1], { &postprocess_render_data->bloom_fx_data.intensity },
					{ { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) } });

				std::vector<VkWriteDescriptorSet> desc_writes;
				std::array<VkDescriptorImageInfo, 2> desc_image_infos{};

				addImageDescriptorSet(desc_writes, desc_image_infos[1], m_blurred_texture_sampler, 0);
				addImageDescriptorSet(desc_writes, desc_image_infos[0], *postprocess_render_data->p_color_texture, 1);

				VulkanRHI::get().getVkCmdPushDescriptorSetKHR()(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_pipeline_layouts[1], 0, static_cast<uint32_t>(desc_writes.size()), desc_writes.data());
				vkCmdDraw(command_buffer, 3, 1, 0, 0);
			}
			vkCmdEndRenderPass(command_buffer);
		}
	}

	void BloomPass::destroy()
	{
		// Brightness
		{
			if (m_brightness_pass.render_pass)
			{
				vkDestroyRenderPass(VulkanRHI::get().getDevice(), m_brightness_pass.render_pass, nullptr);
			}

			for (VkDescriptorSetLayout desc_set_layout : m_brightness_pass.desc_set_layouts)
			{
				vkDestroyDescriptorSetLayout(VulkanRHI::get().getDevice(), desc_set_layout, nullptr);
			}
			for (VkPipelineLayout pipeline_layout : m_brightness_pass.pipeline_layouts)
			{
				vkDestroyPipelineLayout(VulkanRHI::get().getDevice(), pipeline_layout, nullptr);
			}
			for (VkPipeline pipeline : m_brightness_pass.pipelines)
			{
				vkDestroyPipeline(VulkanRHI::get().getDevice(), pipeline, nullptr);
			}
		}

		// Vert Blur
		{
			if (m_vert_blurred_pass.render_pass)
			{
				vkDestroyRenderPass(VulkanRHI::get().getDevice(), m_vert_blurred_pass.render_pass, nullptr);
			}

			for (VkDescriptorSetLayout desc_set_layout : m_vert_blurred_pass.desc_set_layouts)
			{
				vkDestroyDescriptorSetLayout(VulkanRHI::get().getDevice(), desc_set_layout, nullptr);
			}
			for (VkPipelineLayout pipeline_layout : m_vert_blurred_pass.pipeline_layouts)
			{
				vkDestroyPipelineLayout(VulkanRHI::get().getDevice(), pipeline_layout, nullptr);
			}
			for (VkPipeline pipeline : m_vert_blurred_pass.pipelines)
			{
				vkDestroyPipeline(VulkanRHI::get().getDevice(), pipeline, nullptr);
			}
		}

		RenderPass::destroy();
	}

	void BloomPass::createRenderPass()
	{
		// Brightness
		{
			std::vector<VkAttachmentDescription> attachments(1);
			attachments[0].format = m_format;
			attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference reference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			// subpass
			VkSubpassDescription subpass_desc{};
			subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_desc.colorAttachmentCount = 1;
			subpass_desc.pColorAttachments = &reference;

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
					VK_DEPENDENCY_BY_REGION_BIT,
				},
			};

			// create render pass
			VkRenderPassCreateInfo render_pass_ci{};
			render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			render_pass_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
			render_pass_ci.pAttachments = attachments.data();
			render_pass_ci.subpassCount = 1;
			render_pass_ci.pSubpasses = &subpass_desc;
			render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
			render_pass_ci.pDependencies = dependencies.data();

			VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_brightness_pass.render_pass);
			CHECK_VULKAN_RESULT(result, "create vert blurred render pass");
		}

		// Vert Blurred
		{
			std::vector<VkAttachmentDescription> attachments(1);
			attachments[0].format = m_format;
			attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference reference { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			// subpass
			VkSubpassDescription subpass_desc{};
			subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_desc.colorAttachmentCount = 1;
			subpass_desc.pColorAttachments = &reference;

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
					VK_DEPENDENCY_BY_REGION_BIT,
				},
			};

			// create render pass
			VkRenderPassCreateInfo render_pass_ci{};
			render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			render_pass_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
			render_pass_ci.pAttachments = attachments.data();
			render_pass_ci.subpassCount = 1;
			render_pass_ci.pSubpasses = &subpass_desc;
			render_pass_ci.dependencyCount = static_cast<uint32_t>(dependencies.size());
			render_pass_ci.pDependencies = dependencies.data();

			VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_vert_blurred_pass.render_pass);
			CHECK_VULKAN_RESULT(result, "create vert blurred render pass");
		}

		// Final Bloom
		{
			std::vector<VkAttachmentDescription> attachments(2);
			attachments[0].format = m_format;
			attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachments[1].format = m_format;
			attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			std::vector<VkAttachmentReference> references = {
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
				{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			};

			VkAttachmentReference input_reference = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

			// subpass
			std::vector<VkSubpassDescription> subpass_descs(2);
			subpass_descs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_descs[0].colorAttachmentCount = 1;
			subpass_descs[0].pColorAttachments = &references[1];

			subpass_descs[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_descs[1].inputAttachmentCount = 1;
			subpass_descs[1].pInputAttachments = &input_reference;
			subpass_descs[1].colorAttachmentCount = 1;
			subpass_descs[1].pColorAttachments = &references[0];

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
					1,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_DEPENDENCY_BY_REGION_BIT,
				},
				{
					1,
					VK_SUBPASS_EXTERNAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_DEPENDENCY_BY_REGION_BIT,
				},
			};

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
			CHECK_VULKAN_RESULT(result, "create vert blurred render pass");
		}
	}

	void BloomPass::createDescriptorSetLayouts()
	{
		// Brightness
		{
			m_brightness_pass.desc_set_layouts.resize(1);

			// vert blurred descriptor set
			VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
			desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

			std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
				{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			};

			desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
			desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
			VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_brightness_pass.desc_set_layouts[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess descriptor set layout");
		}

		// Vert Blurred
		{
			m_vert_blurred_pass.desc_set_layouts.resize(1);

			// vert blurred descriptor set
			VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
			desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

			std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
				{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			};

			desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
			desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
			VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_vert_blurred_pass.desc_set_layouts[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess descriptor set layout");
		}

		// Final Bloom
		{
			m_desc_set_layouts.resize(2);

			// horz blurred descriptor set
			VkDescriptorSetLayoutCreateInfo desc_set_layout_ci{};
			desc_set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desc_set_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;

			std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
				{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			};

			desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
			desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
			VkResult result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess descriptor set layout");

			// final bloom descriptor set
			desc_set_layout_bindings = {
				{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
			};

			desc_set_layout_ci.bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size());
			desc_set_layout_ci.pBindings = desc_set_layout_bindings.data();
			result = vkCreateDescriptorSetLayout(VulkanRHI::get().getDevice(), &desc_set_layout_ci, nullptr, &m_desc_set_layouts[1]);
			CHECK_VULKAN_RESULT(result, "create postprocess descriptor set layout");
		}
	}

	void BloomPass::createPipelineLayouts()
	{
		// Brightness
		{
			m_brightness_pass.pipeline_layouts.resize(1);

			m_push_constant_ranges =
			{
				{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) }
			};

			VkPipelineLayoutCreateInfo pipeline_layout_ci{};
			pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeline_layout_ci.setLayoutCount = 1;
			pipeline_layout_ci.pSetLayouts = &m_brightness_pass.desc_set_layouts[0];
			pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
			pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();

			VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_brightness_pass.pipeline_layouts[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess pipeline layout");
		}

		// Vert Blurred
		{
			m_vert_blurred_pass.pipeline_layouts.resize(1);

			m_push_constant_ranges =
			{
				{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) }
			};

			VkPipelineLayoutCreateInfo pipeline_layout_ci{};
			pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipeline_layout_ci.setLayoutCount = 1;
			pipeline_layout_ci.pSetLayouts = &m_vert_blurred_pass.desc_set_layouts[0];
			pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
			pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();

			VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_vert_blurred_pass.pipeline_layouts[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess pipeline layout");
		}

		// Final Bloom
		{
			m_pipeline_layouts.resize(2);

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

			VkResult result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess pipeline layout");

			m_push_constant_ranges =
			{
				{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) }
			};

			pipeline_layout_ci.pushConstantRangeCount = static_cast<uint32_t>(m_push_constant_ranges.size());
			pipeline_layout_ci.pPushConstantRanges = m_push_constant_ranges.data();
			pipeline_layout_ci.setLayoutCount = 1;
			pipeline_layout_ci.pSetLayouts = &m_desc_set_layouts[1];


			result = vkCreatePipelineLayout(VulkanRHI::get().getDevice(), &pipeline_layout_ci, nullptr, &m_pipeline_layouts[1]);
			CHECK_VULKAN_RESULT(result, "create postprocess pipeline layout");
		}
	}

	void BloomPass::createPipelines()
	{
		// disable culling and depth testing
		m_rasterize_state_ci.cullMode = VK_CULL_MODE_NONE;
		m_depth_stencil_ci.depthTestEnable = VK_FALSE;
		m_depth_stencil_ci.depthWriteEnable = VK_FALSE;
		m_color_blend_attachments[0].blendEnable = VK_FALSE;

		// vertex input state
		VkPipelineVertexInputStateCreateInfo vertex_input_ci{};
		vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		const auto& shader_manager = g_engine.shaderManager();

		// Brightness
		{
			m_brightness_pass.pipelines.resize(1);

			// shader stages
			std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
				shader_manager->getShaderStageCI("screen.vert", VK_SHADER_STAGE_VERTEX_BIT),
				shader_manager->getShaderStageCI("bright_color.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			m_pipeline_ci.renderPass = m_brightness_pass.render_pass;
			m_pipeline_ci.pVertexInputState = &vertex_input_ci;
			m_pipeline_ci.layout = m_brightness_pass.pipeline_layouts[0];
			m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
			m_pipeline_ci.pStages = shader_stage_cis.data();
			m_pipeline_ci.subpass = 0;

			VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_brightness_pass.pipelines[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess graphics pipeline");
		}

		// Vert Blurred
		{
			m_vert_blurred_pass.pipelines.resize(1);

			// shader stages
			std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
				shader_manager->getShaderStageCI("screen.vert", VK_SHADER_STAGE_VERTEX_BIT),
				shader_manager->getShaderStageCI("bloom.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			m_pipeline_ci.renderPass = m_vert_blurred_pass.render_pass;
			m_pipeline_ci.pVertexInputState = &vertex_input_ci;
			m_pipeline_ci.layout = m_vert_blurred_pass.pipeline_layouts[0];
			m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
			m_pipeline_ci.pStages = shader_stage_cis.data();
			m_pipeline_ci.subpass = 0;

			VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_vert_blurred_pass.pipelines[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess graphics pipeline");
		}

		// Final Bloom
		{
			m_pipelines.resize(2);

			// shader stages
			std::vector<VkPipelineShaderStageCreateInfo> shader_stage_cis = {
				shader_manager->getShaderStageCI("screen.vert", VK_SHADER_STAGE_VERTEX_BIT),
				shader_manager->getShaderStageCI("bloom.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			m_pipeline_ci.renderPass = m_render_pass;
			m_pipeline_ci.pVertexInputState = &vertex_input_ci;
			m_pipeline_ci.layout = m_pipeline_layouts[0];
			m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
			m_pipeline_ci.pStages = shader_stage_cis.data();
			m_pipeline_ci.subpass = 0;

			VkResult result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[0]);
			CHECK_VULKAN_RESULT(result, "create postprocess graphics pipeline");

			shader_stage_cis = {
				shader_manager->getShaderStageCI("screen.vert", VK_SHADER_STAGE_VERTEX_BIT),
				shader_manager->getShaderStageCI("bloom_combine.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
			};
			m_pipeline_ci.layout = m_pipeline_layouts[1];
			m_pipeline_ci.stageCount = static_cast<uint32_t>(shader_stage_cis.size());
			m_pipeline_ci.pStages = shader_stage_cis.data();
			m_pipeline_ci.subpass = 1;

			result = vkCreateGraphicsPipelines(VulkanRHI::get().getDevice(), m_pipeline_cache, 1, &m_pipeline_ci, nullptr, &m_pipelines[1]);
			CHECK_VULKAN_RESULT(result, "create postprocess graphics pipeline");
		}
	}

	void BloomPass::createFramebuffer()
	{
		// Brightness
		{
			// 1.create color images and view
			VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_format,
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_brightness_pass.brightness_texture_sampler,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

			std::vector<VkImageView> attachments =
			{
				m_brightness_pass.brightness_texture_sampler.view
			};

			// 2.create framebuffer
			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = m_brightness_pass.render_pass;
			framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_ci.pAttachments = attachments.data();
			framebuffer_ci.width = m_width;
			framebuffer_ci.height = m_height;
			framebuffer_ci.layers = 1;

			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_brightness_pass.frame_buffer);
			CHECK_VULKAN_RESULT(result, "create postprocess pass frame buffer");
		}

		// Vert Blurred
		{
			// 1.create color images and view
			VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_format,
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_vert_blurred_pass.vert_blurred_texture_sampler,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

			std::vector<VkImageView> attachments =
			{
				m_vert_blurred_pass.vert_blurred_texture_sampler.view
			};

			// 2.create framebuffer
			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = m_vert_blurred_pass.render_pass;
			framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_ci.pAttachments = attachments.data();
			framebuffer_ci.width = m_width;
			framebuffer_ci.height = m_height;
			framebuffer_ci.layers = 1;

			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_vert_blurred_pass.frame_buffer);
			CHECK_VULKAN_RESULT(result, "create postprocess pass frame buffer");
		}

		// Final Bloom
		{
			// 1.create color images and view
			VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_format,
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_color_texture_sampler,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

			VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, 1, 1, m_format,
				VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_blurred_texture_sampler,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

			std::vector<VkImageView> attachments =
			{
				m_color_texture_sampler.view,
				m_blurred_texture_sampler.view
			};

			// 2.create framebuffer
			VkFramebufferCreateInfo framebuffer_ci{};
			framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_ci.renderPass = m_render_pass;
			framebuffer_ci.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_ci.pAttachments = attachments.data();
			framebuffer_ci.width = m_width;
			framebuffer_ci.height = m_height;
			framebuffer_ci.layers = 1;

			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffer);
			CHECK_VULKAN_RESULT(result, "create postprocess pass frame buffer");
		}
	}

	void BloomPass::destroyResizableObjects()
	{
		m_brightness_pass.brightness_texture_sampler.destroy();
		m_vert_blurred_pass.vert_blurred_texture_sampler.destroy();

		m_blurred_texture_sampler.destroy();
		m_color_texture_sampler.destroy();

		if (m_brightness_pass.frame_buffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(VulkanRHI::get().getDevice(), m_brightness_pass.frame_buffer, nullptr);
		}

		if (m_vert_blurred_pass.frame_buffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(VulkanRHI::get().getDevice(), m_vert_blurred_pass.frame_buffer, nullptr);
		}

		RenderPass::destroyResizableObjects();
	}

}