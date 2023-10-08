#include "ui_pass.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/core/base/macro.h"
#include "engine/core/event/event_system.h"
#include "engine/function/render/window_system.h"
#include "engine/platform/timer/timer.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/font/IconsFontAwesome5.h>

namespace Bamboo
{
	void checkVkResult(VkResult result)
	{
		CHECK_VULKAN_RESULT(result, "handle imgui");
	}

	void UIPass::init()
	{
		StopWatch stop_watch;
		stop_watch.start();

		// setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.IniFilename = nullptr;

		// setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// create descriptor pool
		createDescriptorPool();

		// create render pass
		RenderPass::init();

		// setup platform/renderer backends
		ImGui_ImplGlfw_InitForVulkan(g_engine.windowSystem()->getWindow(), true);
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = VulkanRHI::get().getInstance();
		init_info.PhysicalDevice = VulkanRHI::get().getPhysicalDevice();
		init_info.Device = VulkanRHI::get().getDevice();
		init_info.QueueFamily = VulkanRHI::get().getGraphicsQueueFamily();
		init_info.Queue = VulkanRHI::get().getGraphicsQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_descriptor_pool;
		init_info.Subpass = 0;
		init_info.MinImageCount = VulkanRHI::get().getSwapchainImageCount();
		init_info.ImageCount = VulkanRHI::get().getSwapchainImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = &checkVkResult;
		bool is_success = ImGui_ImplVulkan_Init(&init_info, m_render_pass);
		ASSERT(is_success, "failed to init imgui");

		// add consola font
		const float k_base_font_size = 14.0f;
		const float k_icon_font_size = k_base_font_size * 0.8f;
		const auto& fs = g_engine.fileSystem();
		io.Fonts->AddFontFromFileTTF(fs->absolute("asset/engine/font/consola.ttf").c_str(), k_base_font_size);

		// add icon font
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = k_icon_font_size;
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
		io.Fonts->AddFontFromFileTTF(fs->absolute("asset/engine/font/fa-solid-900.ttf").c_str(), k_icon_font_size, &icons_config, icons_ranges);

		// add small consola font
		const float k_small_font_size = 12.0f;
		io.Fonts->AddFontFromFileTTF(fs->absolute("asset/engine/font/consola.ttf").c_str(), k_small_font_size);

		// add big icon font
		const float k_big_icon_font_size = 18.0f;
		icons_config.MergeMode = false;
		icons_config.GlyphMinAdvanceX = k_big_icon_font_size;
		io.Fonts->AddFontFromFileTTF(fs->absolute("asset/engine/font/fa-solid-900.ttf").c_str(), k_big_icon_font_size, &icons_config, icons_ranges);

		// upload fonts
		VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();
		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
		VulkanUtil::endInstantCommands(command_buffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		// create swapchain related objects
		const VkExtent2D& extent = VulkanRHI::get().getSwapchainImageSize();
		createResizableObjects(extent.width, extent.height);

		LOG_INFO("ui pass init time: {}ms", stop_watch.stop());
	}

	void UIPass::prepare()
	{
		// process imgui frame and get draw data
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// set docking over viewport
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dockspace_flags);

		// construct imgui widgets
		g_engine.eventSystem()->syncDispatch(std::make_shared<RenderConstructUIEvent>());

		// calculate imgui draw data
		ImGui::Render();
	}

	void UIPass::render()
	{
		VkCommandBuffer command_buffer = VulkanRHI::get().getCommandBuffer();
		uint32_t flight_index = VulkanRHI::get().getFlightIndex();

		// record render pass
		VkRenderPassBeginInfo renderpass_bi{};
		renderpass_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpass_bi.renderPass = m_render_pass;
		renderpass_bi.framebuffer = m_framebuffers[VulkanRHI::get().getImageIndex()];
		renderpass_bi.renderArea.offset = { 0, 0 };
		renderpass_bi.renderArea.extent = { m_width, m_height };

		VkClearValue clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
		renderpass_bi.clearValueCount = 1;
		renderpass_bi.pClearValues = &clear_value;

		vkCmdBeginRenderPass(command_buffer, &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);

		// record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

		vkCmdEndRenderPass(command_buffer);
	}

	void UIPass::destroy()
	{
		// destroy imgui
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		RenderPass::destroy();
	}

	void UIPass::createRenderPass()
	{
		// create renderpass
		VkAttachmentDescription attachment{};
		attachment.format = VulkanRHI::get().getColorFormat();
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
		dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_ci{};
		render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_ci.attachmentCount = 1;
		render_pass_ci.pAttachments = &attachment;
		render_pass_ci.subpassCount = 1;
		render_pass_ci.pSubpasses = &subpass;
		render_pass_ci.dependencyCount = 1;
		render_pass_ci.pDependencies = &dependency;
		VkResult result = vkCreateRenderPass(VulkanRHI::get().getDevice(), &render_pass_ci, nullptr, &m_render_pass);
		CHECK_VULKAN_RESULT(result, "create imgui render pass");
	}

	void UIPass::createDescriptorPool()
	{
		const uint32_t k_max_image_count = 128;
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k_max_image_count }
		};
		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = k_max_image_count;
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		VkResult result = vkCreateDescriptorPool(VulkanRHI::get().getDevice(), &pool_info, nullptr, &m_descriptor_pool);
		CHECK_VULKAN_RESULT(result, "create imgui descriptor pool");
	}

	void UIPass::createFramebuffer()
	{
		// create framebuffers
		VkImageView image_view;
		VkFramebufferCreateInfo framebuffer_ci{};
		framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_ci.renderPass = m_render_pass;
		framebuffer_ci.attachmentCount = 1;
		framebuffer_ci.pAttachments = &image_view;
		framebuffer_ci.width = m_width;
		framebuffer_ci.height = m_height;
		framebuffer_ci.layers = 1;

		uint32_t image_count = VulkanRHI::get().getSwapchainImageCount();
		m_framebuffers.resize(image_count);
		for (uint32_t i = 0; i < image_count; ++i)
		{
			image_view = VulkanRHI::get().getSwapchainImageViews()[i];
			VkResult result = vkCreateFramebuffer(VulkanRHI::get().getDevice(), &framebuffer_ci, nullptr, &m_framebuffers[i]);
			CHECK_VULKAN_RESULT(result, "create imgui frame buffer");
		}
	}

	void UIPass::destroyResizableObjects()
	{
		for (VkFramebuffer framebuffer : m_framebuffers)
		{
			vkDestroyFramebuffer(VulkanRHI::get().getDevice(), framebuffer, nullptr);
		}
		m_framebuffers.clear();
	}

}