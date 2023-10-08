#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class BRDFLUTPass : public RenderPass
	{
	public:
		BRDFLUTPass();

		virtual void init() override;
		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

	private:
		VkFormat m_format;
		uint32_t m_size;
		VmaImageView m_image_view;
	};
}