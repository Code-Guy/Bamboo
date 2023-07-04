#include "simulation_ui.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/pass/base_pass.h"

#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/transform_component.h"

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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (!ImGui::Begin(combine(ICON_FA_GAMEPAD, m_title).c_str()))
		{
			ImGui::End();
			return;
		}
		checkWindowResize();

		ImVec2 content_size = ImGui::GetContentRegionAvail();
		ImGui::Image(m_color_texture_desc_set, ImVec2{content_size.x, content_size.y});

		// allow drag from asset ui
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("load_asset"))
			{
				std::string url((const char*)payload->Data, payload->DataSize);
				LOG_INFO("loading asset: {}", url);
				loadAsset(url);
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
		std::shared_ptr<RenderPass> render_pass = g_runtime_context.renderSystem()->getRenderPass(ERenderPassType::Base);
		std::shared_ptr<BasePass> base_pass = std::dynamic_pointer_cast<BasePass>(render_pass);
		base_pass->onResize(m_width, m_height);

		if (m_color_texture_desc_set != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
		}
		m_color_texture_desc_set = ImGui_ImplVulkan_AddTexture(m_color_texture_sampler, base_pass->getColorImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void SimulationUI::loadAsset(const std::string& url)
	{
		EAssetType asset_type = g_runtime_context.assetManager()->getAssetType(url);
		std::string basename = g_runtime_context.fileSystem()->basename(url);

		if (asset_type == EAssetType::StaticMesh)
		{
			std::shared_ptr<World> world = g_runtime_context.worldManager()->getCurrentWorld();
			std::shared_ptr<Entity> static_mesh_entity = world->createEntity(basename);

			std::shared_ptr<StaticMeshComponent> static_mesh_component = std::make_shared<StaticMeshComponent>();
			std::shared_ptr<StaticMesh> static_mesh = g_runtime_context.assetManager()->loadAsset<StaticMesh>(url);
			static_mesh_component->setStaticMesh(static_mesh);
			static_mesh_entity->addComponent(static_mesh_component);

			std::shared_ptr<TransformComponent> transform_component = std::make_shared<TransformComponent>();
			static_mesh_entity->addComponent(transform_component);
		}
	}

}