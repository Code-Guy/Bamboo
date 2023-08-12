#include "simulation_ui.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/pass/base_pass.h"
#include "runtime/function/render/pass/gbuffer_pass.h"

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
		// reset camera aspect ratio
		g_runtime_context.worldManager()->getCameraComponent()->setContentRegion(m_content_region);

		// resize render pass
		std::shared_ptr<BasePass> base_pass = std::dynamic_pointer_cast<BasePass>(g_runtime_context.renderSystem()->getRenderPass(ERenderPassType::Base));
		std::shared_ptr<GBufferPass> gbuffer_pass = std::dynamic_pointer_cast<GBufferPass>(g_runtime_context.renderSystem()->getRenderPass(ERenderPassType::Gbuffer));
		base_pass->onResize(m_content_region.z, m_content_region.w);
		gbuffer_pass->onResize(m_content_region.z, m_content_region.w);

		// recreate color image and view
		if (m_color_texture_desc_set != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(m_color_texture_desc_set);
		}
		m_color_texture_desc_set = ImGui_ImplVulkan_AddTexture(m_color_texture_sampler, base_pass->getColorImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void SimulationUI::onWindowRepos()
	{
		g_runtime_context.worldManager()->getCameraComponent()->setContentRegion(m_content_region);
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
		entity->setTickEnabled(true);

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

			entity->setTickInterval(0.0167f);
		}
	}

}