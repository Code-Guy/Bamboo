#include "game_ui.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/base_pass.h"
#include <imgui/backends/imgui_impl_vulkan.h>

namespace Bamboo
{

	void GameUI::init()
	{
		m_title = "Game";
		m_color_texture_sampler = createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 1, VK_SAMPLER_ADDRESS_MODE_REPEAT, 
			VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}

	void GameUI::construct()
	{
		EditorUI::construct();

		ImGui::Begin(m_title.c_str());
		ImVec2 content_size = ImGui::GetContentRegionAvail();
		ImGui::Image(m_color_texture_desc_set, ImVec2{content_size.x, content_size.y});
		ImGui::End();
	}

	void GameUI::destroy()
	{
		vkDestroySampler(VulkanRHI::get().getDevice(), m_color_texture_sampler, nullptr);
	}

	void GameUI::on_window_resize()
	{
		std::shared_ptr<RenderPass> render_pass = VulkanRHI::get().getRenderPasses()[ERenderPassType::Base];
		std::shared_ptr<BasePass> base_pass = std::dynamic_pointer_cast<BasePass>(render_pass);
		base_pass->on_resize(m_width, m_height);

		if (m_color_texture_desc_set != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
		}
		m_color_texture_desc_set = ImGui_ImplVulkan_AddTexture(m_color_texture_sampler, base_pass->getColorImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

}