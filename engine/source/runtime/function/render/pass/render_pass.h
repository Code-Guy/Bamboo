#pragma once

#include "runtime/core/vulkan/vulkan_util.h"

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
		virtual void prepare() = 0;
		virtual void record() = 0;
		virtual void destroy();

		virtual void createRenderPass() = 0;
		virtual void createDescriptorPool() = 0;
		virtual void createDescriptorSetLayout() = 0;
		virtual void createPipelineLayout() = 0;
		virtual void createPipeline() = 0;
		virtual void createFramebuffer() = 0;
		virtual void createResizableObjects(uint32_t width, uint32_t height);
		virtual void destroyResizableObjects() {}

		void onResize(uint32_t width, uint32_t height);
		bool isMinimize();

	protected:
		VkRenderPass m_render_pass;
		VkDescriptorPool m_descriptor_pool;
		VkDescriptorSetLayout m_descriptor_set_layout;
		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_pipeline;

		uint32_t m_width = 0, m_height = 0;
	};
}