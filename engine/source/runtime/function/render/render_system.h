#pragma once

#include "pass/render_pass.h"

#include <map>
#include <memory>
#include <functional>

namespace Bamboo
{
	class RenderSystem
	{
	public:
		void init();
		void tick(float delta_time);
		void destroy();

		void setConstructUIFunc(const std::function<void()>& construct_ui_func);
		std::shared_ptr<RenderPass> getRenderPass(ERenderPassType render_pass_type);

	private:
		void onCreateSwapchainObjects(uint32_t width, uint32_t height);
		void onDestroySwapchainObjects();
		void onRecordFrame(VkCommandBuffer command_buffer, uint32_t flight_index);

		void collectRenderDatas();

		std::map<RenderDataID, std::shared_ptr<RenderData>> m_cached_render_datas;
		std::map<ERenderPassType, std::shared_ptr<RenderPass>> m_render_passes;
	};
}