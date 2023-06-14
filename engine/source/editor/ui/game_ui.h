#pragma once

#include "editor_ui.h"
#include "runtime/core/vulkan/vulkan_util.h"

namespace Bamboo
{
	class GameUI : public EditorUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;
		virtual void on_window_resize() override;

	private:
		VkSampler m_color_texture_sampler;
		VkDescriptorSet m_color_texture_desc_set = VK_NULL_HANDLE;
	};
}