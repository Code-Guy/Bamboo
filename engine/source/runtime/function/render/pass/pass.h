#pragma once

#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{
	class Pass
	{
	public:
		virtual void init() = 0;
		virtual void render() = 0;
		virtual void destroy() = 0;
		virtual void createSwapchainObjects() {}
		virtual void destroySwapchainObjects() {}

	protected:
		VkDescriptorPool m_descriptor_pool;
		VkRenderPass m_render_pass;
		std::vector<VkFramebuffer> m_framebuffers;
	};
}