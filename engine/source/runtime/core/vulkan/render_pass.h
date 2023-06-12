#pragma once

#include "vulkan_util.h"

namespace Bamboo
{
	class RenderPass
	{
	public:
		virtual void init() = 0;
		virtual void prepare() = 0;
		virtual void record() = 0;
		virtual void destroy() = 0;
		virtual void createSwapchainObjects() {}
		virtual void destroySwapchainObjects() {}

	protected:
		VkDescriptorPool m_descriptor_pool;
		VkRenderPass m_render_pass;
		std::vector<VkFramebuffer> m_framebuffers;
	};
}