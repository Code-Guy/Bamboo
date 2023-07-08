#pragma once

#include "runtime/function/render/render_data.h"

namespace Bamboo
{
	enum class ERenderPassType
	{
		Base, UI
	};

	class RenderPass
	{
	public:
		virtual void init() = 0;
		virtual void prepare() {}
		virtual void render(VkCommandBuffer command_buffer, uint32_t flight_index) = 0;
		virtual void destroy();

		virtual void createRenderPass() = 0;
		virtual void createDescriptorSetLayout() = 0;
		virtual void createPipelineLayout() = 0;
		virtual void createPipeline() = 0;
		virtual void createFramebuffer() = 0;
		virtual void createResizableObjects(uint32_t width, uint32_t height);
		virtual void destroyResizableObjects() {}

		void setRenderDatas(const std::vector<std::shared_ptr<RenderData>>& render_datas) { m_render_datas = render_datas; }
		void onResize(uint32_t width, uint32_t height);
		bool isMinimize();

	protected:
		// vulkan objects
		VkRenderPass m_render_pass;
		VkDescriptorPool m_descriptor_pool;
		VkDescriptorSetLayout m_desc_set_layout;
		std::vector<VkPushConstantRange> m_push_constant_ranges;
		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_pipeline;

		// render dependent data
		std::vector<std::shared_ptr<RenderData>> m_render_datas;

		// render target size
		uint32_t m_width = 0, m_height = 0;
	};
}