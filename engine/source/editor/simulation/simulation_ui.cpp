#include "simulation_ui.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/pass/main_pass.h"

#include "runtime/platform/timer/timer.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/static_mesh_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"
#include "runtime/function/framework/component/animation_component.h"
#include "runtime/function/framework/component/animator_component.h"

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
		const std::string& world_name = g_runtime_context.worldManager()->getCurrentWorldName();
		sprintf(m_title_buf, "%s %s###%s", ICON_FA_GAMEPAD, world_name.c_str(), m_title.c_str());
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (!ImGui::Begin(m_title_buf))
		{
			ImGui::End();
			ImGui::PopStyleVar();
			return;
		}
		updateWindowRegion();

		ImVec2 content_size = ImGui::GetContentRegionAvail();
		ImGui::Image(m_color_texture_desc_set, ImVec2{content_size.x, content_size.y});

		// allow drag from asset ui
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("load_asset"))
			{
				std::string url((const char*)payload->Data, payload->DataSize);
				StopWatch stop_watch;
				stop_watch.start();
				loadAsset(url);
				LOG_INFO("load asset {}, elapsed time: {}ms", url, stop_watch.stop());
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
		ImGui::SetCursorPos(ImVec2(10, 30));
		sprintf(m_title_buf, "%s view", ICON_FA_DICE_D6);
		if (ImGui::Button(m_title_buf, ImVec2(64, 24)))
		{
			ImGui::OpenPopup("view");
		}

		static int view_index = 0;
		static const std::vector<std::string> views = {
			"perspective", "top", "bottom", "left", "right", "front", "back"
		};
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-2.0f, -2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
		constructRadioButtonPopup("view", views, view_index);

		ImGui::SameLine();
		sprintf(m_title_buf, "%s shader", ICON_FA_BOWLING_BALL);
		if (ImGui::Button(m_title_buf, ImVec2(75, 24)))
		{
			ImGui::OpenPopup("shader");
		}

		static int shader_index = 0;
		static const std::vector<std::string> shaders = {
			"lit", "unlit", "wireframe", "lighting only", "depth", "normal", "base color", "emissive color",
			"metallic", "roughness", "occlusion", "opacity"
		};
		constructRadioButtonPopup("shader", shaders, shader_index);

		ImGui::SameLine();
		sprintf(m_title_buf, "%s show", ICON_FA_EYE);
		if (ImGui::Button(m_title_buf, ImVec2(64, 24)))
		{
			ImGui::OpenPopup("show");
		}

		static std::vector<std::pair<std::string, bool>> shows = {
			{ "anti-aliasing", false }, { "bounding boxes", false }, { "collision", false }, { "grid", false }, 
			{ "static Meshes", false }, { "skeletal meshes", false }, { "translucency", false }
		};
		constructCheckboxPopup("show", shows);

		ImGui::PopStyleVar(3);

		ImGui::SameLine(content_size.x - 130);
		if (ImGui::Button(ICON_FA_MOUSE_POINTER, ImVec2(24, 24)))
		{

		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_MOVE, ImVec2(24, 24)))
		{

		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_SYNC_ALT, ImVec2(24, 24)))
		{

		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_EXPAND, ImVec2(24, 24)))
		{

		}

		ImGui::PopStyleColor();

		ImGui::End();
		ImGui::PopStyleVar();

		// set camera component
		auto camera_component = g_runtime_context.worldManager()->getCameraComponent();
		camera_component->m_aspect_ratio = (float)m_content_region.z / m_content_region.w;
		camera_component->m_enabled = !isPoppingUp() && isMouseFocused();
	}

	void SimulationUI::destroy()
	{
		EditorUI::destroy();

		vkDestroySampler(VulkanRHI::get().getDevice(), m_color_texture_sampler, nullptr);
		ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
	}

	void SimulationUI::onWindowResize()
	{
		// resize render pass
		const auto& main_pass = g_runtime_context.renderSystem()->getMainPass();
		main_pass->onResize(m_content_region.z, m_content_region.w);

		// recreate color image and view
		if (m_color_texture_desc_set != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
		}
		m_color_texture_desc_set = ImGui_ImplVulkan_AddTexture(m_color_texture_sampler, main_pass->getColorImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void SimulationUI::loadAsset(const std::string& url)
	{
		const auto& as = g_runtime_context.assetManager();
		EAssetType asset_type = as->getAssetType(url);
		std::string basename = g_runtime_context.fileSystem()->basename(url);

		std::shared_ptr<World> world = g_runtime_context.worldManager()->getCurrentWorld();
		std::shared_ptr<Entity> entity = world->createEntity(basename);

		// add transform component
		std::shared_ptr<TransformComponent> transform_component = std::make_shared<TransformComponent>();
		entity->addComponent(transform_component);

		if (asset_type == EAssetType::StaticMesh)
		{
			std::shared_ptr<StaticMeshComponent> static_mesh_component = std::make_shared<StaticMeshComponent>();
			std::shared_ptr<StaticMesh> static_mesh = as->loadAsset<StaticMesh>(url);
			static_mesh_component->setStaticMesh(static_mesh);
			entity->addComponent(static_mesh_component);
		}
		else if (asset_type == EAssetType::SkeletalMesh)
		{
			std::shared_ptr<SkeletalMeshComponent> skeletal_mesh_component = std::make_shared<SkeletalMeshComponent>();
			std::shared_ptr<SkeletalMesh> skeletal_mesh = as->loadAsset<SkeletalMesh>(url);
			skeletal_mesh_component->setSkeletalMesh(skeletal_mesh);
			entity->addComponent(skeletal_mesh_component);

			std::shared_ptr<AnimationComponent> animation_component = std::make_shared<AnimationComponent>();
			std::shared_ptr<Animation> animation = as->loadAsset<Animation>("asset/cesium_man/anim_Anim_0.anim");
			animation_component->addAnimation(animation);
			entity->addComponent(animation_component);

			std::shared_ptr<AnimatorComponent> animator_component = std::make_shared<AnimatorComponent>();
			std::shared_ptr<Skeleton> skeleton = as->loadAsset<Skeleton>("asset/cesium_man/skl_Armature.skl");
			animator_component->setTickEnabled(true);
			animator_component->setSkeleton(skeleton);
			entity->addComponent(animator_component);

			entity->setTickEnabled(true);
			entity->setTickInterval(0.0167f);
		}
	}

	bool SimulationUI::constructRadioButtonPopup(const std::string& popup_name, const std::vector<std::string>& values, int& index)
	{
		bool is_radio_button_pressed = false;
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		if (ImGui::BeginPopup(popup_name.c_str()))
		{
			for (size_t i = 0; i < values.size(); ++i)
			{
				if (ImGui::RadioButton(values[i].c_str(), &index, i))
				{
					is_radio_button_pressed = true;
				}
				if (i != values.size() - 1)
				{
					ImGui::Spacing();
				}
			}
			ImGui::EndPopup();
		}

		return is_radio_button_pressed;
	}

	void SimulationUI::constructCheckboxPopup(const std::string& popup_name, std::vector<std::pair<std::string, bool>>& values)
	{
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		if (ImGui::BeginPopup(popup_name.c_str()))
		{
			for (size_t i = 0; i < values.size(); ++i)
			{
				if (ImGui::Checkbox(values[i].first.c_str(), &values[i].second))
				{
					
				}
				if (i != values.size() - 1)
				{
					ImGui::Spacing();
				}
			}
			ImGui::EndPopup();
		}
	}

}