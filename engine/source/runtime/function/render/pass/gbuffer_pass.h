#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class GBufferPass : public RenderPass
	{
	public:
		GBufferPass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

	private:
		std::vector<VkFormat> m_formats;

		VmaImageViewSampler m_position_texture_sampler;
		VmaImageViewSampler m_normal_texture_sampler;
		VmaImageViewSampler m_base_color_texture_sampler;
		VmaImageViewSampler m_emissive_texture_sampler;
		VmaImageViewSampler m_metallic_roughness_occlusion_texture_sampler;
		VmaImageViewSampler m_depth_stencil_texture_sampler;
	};
}