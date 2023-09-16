#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class PickPass : public RenderPass
	{
	public:
		PickPass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void createResizableObjects(uint32_t width, uint32_t height) override;
		virtual void destroyResizableObjects() override;

		void pick(uint32_t mouse_x, uint32_t mouse_y);
		virtual bool isEnabled() override;

		void setBillboardRenderDatas(const std::vector<std::shared_ptr<BillboardRenderData>>& billboard_render_datas) {
			m_billboard_render_datas = billboard_render_datas;
		}
		void setEntityIDs(const std::vector<uint32_t>& entity_ids) { m_entity_ids = entity_ids; }

	private:
		glm::vec4 encodeEntityID(uint32_t id);
		uint32_t decodeEntityID(const uint8_t* color);

		VkFormat m_formats[2];
		VmaImageView m_color_image_view;
		VmaImageView m_depth_image_view;
		std::vector<VkPushConstantRange> m_billboard_push_constant_ranges;

		std::vector<std::shared_ptr<BillboardRenderData>> m_billboard_render_datas;
		std::vector<uint32_t> m_entity_ids;

		bool m_enabled;
		uint32_t m_mouse_x;
		uint32_t m_mouse_y;
		float m_scale_ratio;
	};
}