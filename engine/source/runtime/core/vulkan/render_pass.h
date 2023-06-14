#pragma once

#include "vulkan_util.h"

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
		virtual void createResizableObjects(uint32_t width, uint32_t height);
		virtual void destroyResizableObjects() {}

		void on_resize(uint32_t width, uint32_t height);
		bool is_minimize();

	protected:
		VkDescriptorPool m_descriptor_pool;
		VkRenderPass m_render_pass;
		uint32_t m_width = 0, m_height = 0;
	};
}