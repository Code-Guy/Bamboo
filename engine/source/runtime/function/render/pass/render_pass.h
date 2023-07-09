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
		virtual void createDescriptorSetLayouts() = 0;
		virtual void createPipelineLayouts() = 0;
		virtual void createPipelines() = 0;
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
		std::vector<VkDescriptorSetLayout> m_desc_set_layouts;
		std::vector<VkPushConstantRange> m_push_constant_ranges;
		std::vector<VkPipelineLayout> m_pipeline_layouts;
		VkPipelineCache m_pipeline_cache;
		std::vector<VkPipeline> m_pipelines;
		std::vector<VkFramebuffer> m_framebuffers;
		VkFramebuffer m_framebuffer;

		// render dependent data
		std::vector<std::shared_ptr<RenderData>> m_render_datas;

		// render target size
		uint32_t m_width = 0, m_height = 0;
	};
}