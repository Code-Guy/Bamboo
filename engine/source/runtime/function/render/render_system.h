#pragma once

#include "runtime/function/render/pass/render_pass.h"

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

		const std::shared_ptr<class MainPass>& getMainPass() { return m_main_pass; }

	private:
		void onCreateSwapchainObjects(const std::shared_ptr<class Event>& event);
		void onDestroySwapchainObjects(const std::shared_ptr<class Event>& event);
		void onRecordFrame(const std::shared_ptr<class Event>& event);

		void collectRenderDatas();
		std::shared_ptr<BillboardRenderData> createBillboardRenderData(
			std::shared_ptr<class TransformComponent> transform_component,
			std::shared_ptr<class CameraComponent> camera_component,
			ELightType light_type);

		std::shared_ptr<class Texture2D> m_default_texture_2d;
		std::shared_ptr<class TextureCube> m_default_texture_cube;

		std::shared_ptr<class DirectionalLightShadowPass> m_directional_light_shadow_pass;
		std::shared_ptr<class PointLightShadowPass> m_point_light_shadow_pass;
		std::shared_ptr<class SpotLightShadowPass> m_spot_light_shadow_pass;
		std::shared_ptr<class MainPass> m_main_pass;
		std::shared_ptr<class UIPass> m_ui_pass;
		std::vector<std::shared_ptr<RenderPass>> m_render_passes;

		std::vector<VmaBuffer> m_lighting_ubs;
		std::map<ELightType, std::shared_ptr<class Texture2D>> m_lighting_icons;

		// pick entity
		bool m_picking_entity;
	};
}