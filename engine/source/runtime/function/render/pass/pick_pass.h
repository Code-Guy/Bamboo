#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class PickPass : public RenderPass
	{
	public:
		PickPass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

	private:
		VkFormat m_formats[2];
		VmaImageView m_color_image_view;
		VmaImageView m_depth_image_view;
	};
}