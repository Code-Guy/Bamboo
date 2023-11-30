#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class PostprocessPass : public RenderPass
	{
	public:
		PostprocessPass();

		virtual void init() override;
		virtual void render() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		VmaImageViewSampler getColorTexture() { return m_color_texture_sampler; }

	private:
		void loadColorGradingTexture(const std::string& filename);

		VkFormat m_format;
		VmaImageViewSampler m_color_texture_sampler;
		VmaImageViewSampler m_color_grading_texture_sampler;
		VmaImageViewSampler m_color_outline_texture_sampler;

		struct PostprocessData
		{
			float exposure;
			float intensity;
			float threshold;
		};
	};
}