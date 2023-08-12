#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class DeferredLightingPass : public RenderPass
	{
	public:
		DeferredLightingPass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

	private:
		VkFormat m_format;
		VmaImageView m_image_view;
	};
}