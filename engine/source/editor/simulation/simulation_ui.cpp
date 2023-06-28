#include "simulation_ui.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/base_pass.h"
#include <imgui/backends/imgui_impl_vulkan.h>

namespace Bamboo
{

	void SimulationUI::init()
	{
		m_title = "Simulation";
		m_color_texture_sampler = VulkanUtil::createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 1, VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	}

	void SimulationUI::construct()
	{
		EditorUI::construct();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (!ImGui::Begin(combine(ICON_FA_GAMEPAD, m_title).c_str()))
		{
			ImGui::End();
			return;
		}

		ImVec2 content_size = ImGui::GetContentRegionAvail();
		ImGui::Image(m_color_texture_desc_set, ImVec2{content_size.x, content_size.y});

		// allow drag from asset ui
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("load_asset"))
			{
				std::string asset_url((const char*)payload->Data, payload->DataSize);
				LOG_INFO("load asset: {}", asset_url);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SimulationUI::destroy()
	{
		EditorUI::destroy();

		vkDestroySampler(VulkanRHI::get().getDevice(), m_color_texture_sampler, nullptr);
		ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
	}

	void SimulationUI::onWindowResize()
	{
		std::shared_ptr<RenderPass> render_pass = VulkanRHI::get().getRenderPasses()[ERenderPassType::Base];
		std::shared_ptr<BasePass> base_pass = std::dynamic_pointer_cast<BasePass>(render_pass);
		base_pass->onResize(m_width, m_height);

		if (m_color_texture_desc_set != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
		}
		m_color_texture_desc_set = ImGui_ImplVulkan_AddTexture(m_color_texture_sampler, base_pass->getColorImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

}