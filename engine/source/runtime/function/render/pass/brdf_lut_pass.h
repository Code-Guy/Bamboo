#pragma once

#include "render_pass.h"

#define BRDF_TEX_URL "asset/engine/texture/ibl/tex_brdf_lut.tex"

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

	private:
		VkFormat m_format;
		VmaImageView m_color_image_view;
		VmaImage m_staging_image;
	};
}