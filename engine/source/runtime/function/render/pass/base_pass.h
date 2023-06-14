#pragma once

#include "runtime/core/vulkan/render_pass.h"

namespace Bamboo
{
	class BasePass : public RenderPass
	{
	public:
		virtual void init() override;
		virtual void prepare() override;
		virtual void record() override;
		virtual void destroy() override;
		virtual void createResizableObjects(uint32_t width, uint32_t height) override;
		virtual void destroyResizableObjects() override;

		VkImageView getColorImageView();

	private:
		VmaImageView m_depth_stencil_image_view;
		VmaImageView m_color_image_view;
		VkFramebuffer m_framebuffer;
	};
}