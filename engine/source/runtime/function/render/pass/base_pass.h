#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class BasePass : public RenderPass
	{
	public:
		virtual void init() override;
		virtual void render(VkCommandBuffer command_buffer, uint32_t flight_index) override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void createResizableObjects(uint32_t width, uint32_t height) override;
		virtual void destroyResizableObjects() override;

		VkImageView getColorImageView();

	private:
		VmaImageView m_depth_stencil_image_view;
		VmaImageView m_color_image_view;
	};
}