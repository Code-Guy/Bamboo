#include "ui_pass.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>

namespace Bamboo
{
	void checkVkResult(VkResult result)
	{
		CHECK_VULKAN_RESULT(result, "handle imgui");
	}

	void UIPass::init()
	{
		// setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		// setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// create descriptor pool
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
		};
		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1;
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		VkResult result = vkCreateDescriptorPool(VulkanRHI::instance().getDevice(), &pool_info, nullptr, &m_descriptor_pool);
		CHECK_VULKAN_RESULT(result, "create imgui descriptor pool");

		// create renderpass
		VkAttachmentDescription attachment{};
		attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment{};
		color_attachment.attachment = 0;
		color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = 1;
		render_pass_ci.pAttachments = &attachment;
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass;
		render_pass_ci.dependencyCount = 1;
		render_pass_ci.pDependencies = &dependency;
		result = vkCreateRenderPass(VulkanRHI::instance().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create imgui render pass");

		// setup platform/renderer backends
		ImGui_ImplGlfw_InitForVulkan(g_runtime_context.windowSystem()->getWindow(), true);
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = VulkanRHI::instance().getInstance();
		init_info.PhysicalDevice = VulkanRHI::instance().getPhysicalDevice();
		init_info.Device = VulkanRHI::instance().getDevice();
		init_info.QueueFamily = VulkanRHI::instance().getGraphicsQueueFamily();
		init_info.Queue = VulkanRHI::instance().getGraphicsQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_descriptor_pool;
		init_info.Subpass = 0;
		init_info.MinImageCount = VulkanRHI::instance().getSwapchainImageCount();
		init_info.ImageCount = VulkanRHI::instance().getSwapchainImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = &checkVkResult;
		ImGui_ImplVulkan_Init(&init_info, m_render_pass);

		// upload fonts
		VkCommandBuffer command_buffer = beginInstantCommands();
		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
		endInstantCommands(command_buffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		// create swapchain related objects
		createSwapchainObjects();
	}

	void UIPass::render()
	{
		// process imgui frame and get draw data
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Render();
		ImDrawData* imgui_draw_data = ImGui::GetDrawData();

		// TODO
		// setup imgui widgets


		// start render pass
		VkRenderPassBeginInfo renderpass_bi{};
		renderpass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpass_bi.renderPass = m_render_pass;
		renderpass_bi.framebuffer = m_framebuffers[VulkanRHI::instance().getImageIndex()];
		renderpass_bi.renderArea.offset = { 0, 0 };
		renderpass_bi.renderArea.extent = VulkanRHI::instance().getSwapchainImageSize();

		VkClearValue clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
		renderpass_bi.clearValueCount = 1;
		renderpass_bi.pClearValues = &clear_value;
		VkCommandBuffer command_buffer = VulkanRHI::instance().getCommandBuffer();
		vkCmdBeginRenderPass(command_buffer, &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);

		// record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(imgui_draw_data, command_buffer);

		vkCmdEndRenderPass(command_buffer);
	}

	void UIPass::destroy()
	{
		// destroy imgui
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// destroy swapchain related objects
		destroySwapchainObjects();

		vkDestroyRenderPass(VulkanRHI::instance().getDevice(), m_render_pass, nullptr);
		vkDestroyDescriptorPool(VulkanRHI::instance().getDevice(), m_descriptor_pool, nullptr);
	}

	void UIPass::createSwapchainObjects()
	{
		// create framebuffers
		VkImageView image_view;
		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = 1;
		framebuffer_ci.pAttachments = &image_view;
		framebuffer_ci.width = VulkanRHI::instance().getSwapchainImageSize().width;
		framebuffer_ci.height = VulkanRHI::instance().getSwapchainImageSize().height;
		framebuffer_ci.layers = 1;
		for (uint32_t i = 0; i < VulkanRHI::instance().getSwapchainImageCount(); ++i)
		{
			image_view = VulkanRHI::instance().getSwapchainImageViews()[i];
			VkResult result = vkCreateFramebuffer(VulkanRHI::instance().getDevice(), &framebuffer_ci, nullptr, &m_framebuffers[i]);
			CHECK_VULKAN_RESULT(result, "create imgui frame buffer");
		}
	}

	void UIPass::destroySwapchainObjects()
	{
		for (VkFramebuffer framebuffer : m_framebuffers)
		{
			vkDestroyFramebuffer(VulkanRHI::instance().getDevice(), framebuffer, nullptr);
		}
	}

}