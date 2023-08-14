#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class MainPass : public RenderPass
	{
	public:
		MainPass();

		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		VkImageView getColorImageView() { return m_color_image_view.view; }

	private:
		enum class ERendererType
		{
			Deferred, Forward
		};

		void render_mesh(std::shared_ptr<RenderData>& render_data, ERendererType renderer_type);

		std::vector<VkFormat> m_formats;

		// color attachment
		VmaImageView m_color_image_view;

		// gbuffer attachment
		VmaImageViewSampler m_position_texture_sampler;
		VmaImageViewSampler m_normal_texture_sampler;
		VmaImageViewSampler m_base_color_texture_sampler;
		VmaImageViewSampler m_emissive_texture_sampler;
		VmaImageViewSampler m_metallic_roughness_occlusion_texture_sampler;

		// depth stencil attachment
		VmaImageViewSampler m_depth_stencil_texture_sampler;
	};
}