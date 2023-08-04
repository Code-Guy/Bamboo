#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class BRDFLUTPass : public RenderPass
	{
	public:
		BRDFLUTPass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		VmaImageViewSampler getColorImageViewSampler() { return m_color_view_sampler; }

	private:
		VkFormat m_format;
		VmaImageViewSampler m_color_view_sampler;
		VmaImage m_staging_image;
	};
}