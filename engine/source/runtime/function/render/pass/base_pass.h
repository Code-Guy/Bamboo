#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class BasePass : public RenderPass
	{
	public:
		virtual void init() override;
		virtual void prepare() override;
		virtual void record() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorPool() override;
		virtual void createDescriptorSetLayout() override;
		virtual void createPipelineLayout() override;
		virtual void createPipeline() override;
		virtual void createFramebuffer() override;
		virtual void createResizableObjects(uint32_t width, uint32_t height) override;
		virtual void destroyResizableObjects() override;

		VkImageView getColorImageView();

	private:
		VmaImageView m_depth_stencil_image_view;
		VmaImageView m_color_image_view;
		VkFramebuffer m_framebuffer;
	};
}