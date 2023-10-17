#pragma once

#include "engine/function/render/pass/render_pass.h"

#include <map>
#include <memory>
#include <functional>

namespace Bamboo
{
	enum class ELightType
	{
		DirectionalLight, SkyLight, PointLight, SpotLight
	};

	class RenderSystem
	{
	public:
		void init();
		void tick(float delta_time);
		void destroy();

		void resize(uint32_t width, uint32_t height);
		void setShaderDebugOption(int option) { m_shader_debug_option = option; }
		void setShowDebugOption(int option) { m_show_debug_option = option; }

		VkImageView getColorImageView();

	private:
		void onCreateSwapchainObjects(const std::shared_ptr<class Event>& event);
		void onDestroySwapchainObjects(const std::shared_ptr<class Event>& event);
		void onRecordFrame(const std::shared_ptr<class Event>& event);
		void onPickEntity(const std::shared_ptr<class Event>& event);
		void onSelectEntity(const std::shared_ptr<class Event>& event);

		void collectRenderDatas();
		void addBillboardRenderData(
			std::shared_ptr<class TransformComponent> transform_component,
			std::shared_ptr<class CameraComponent> camera_component,
			std::vector<std::shared_ptr<BillboardRenderData>>& billboard_render_datas, 
			std::vector<std::shared_ptr<BillboardRenderData>>& selected_billboard_render_datas,
			std::vector<uint32_t>& billboard_entity_ids,
			ELightType light_type);

		// render passes
		std::shared_ptr<class DirectionalLightShadowPass> m_directional_light_shadow_pass;
		std::shared_ptr<class PointLightShadowPass> m_point_light_shadow_pass;
		std::shared_ptr<class SpotLightShadowPass> m_spot_light_shadow_pass;
		std::shared_ptr<class PickPass> m_pick_pass;
		std::shared_ptr<class OutlinePass> m_outline_pass;
		std::shared_ptr<class MainPass> m_main_pass;
		std::shared_ptr<class PostprocessPass> m_postprocess_pass;
		std::shared_ptr<class UIPass> m_ui_pass;
		std::vector<std::shared_ptr<RenderPass>> m_render_passes;

		// render datas
		std::vector<VmaBuffer> m_lighting_ubs;
		std::shared_ptr<class Texture2D> m_default_texture_2d;
		std::shared_ptr<class TextureCube> m_default_texture_cube;
		std::map<ELightType, std::shared_ptr<class Texture2D>> m_lighting_icons;

		// render options
		int m_shader_debug_option = 0;
		int m_show_debug_option = 0;

		// selection
		std::vector<uint32_t> m_selected_entity_ids;
	};
}