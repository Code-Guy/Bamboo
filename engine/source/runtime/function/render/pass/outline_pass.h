#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class OutlinePass : public RenderPass
	{
	public:
		OutlinePass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		virtual bool isEnabled() override;

		void setBillboardRenderDatas(const std::vector<std::shared_ptr<BillboardRenderData>>& billboard_render_datas) {
			m_billboard_render_datas = billboard_render_datas;
		}

	private:
		VkFormat m_format;
		VmaImageView m_color_image_view;

		std::vector<VkPushConstantRange> m_billboard_push_constant_ranges;
		std::vector<std::shared_ptr<BillboardRenderData>> m_billboard_render_datas;
	};
}