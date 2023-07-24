#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class BasePass : public RenderPass
	{
	public:
		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		VkImageView getColorImageView();

	private:
		VmaImageView m_depth_stencil_image_view;
		VmaImageView m_color_image_view;
	};
}