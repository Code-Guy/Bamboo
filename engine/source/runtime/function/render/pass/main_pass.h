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

		void setLightingRenderData(const std::shared_ptr<LightingRenderData>& lighting_render_data) { m_lighting_render_data = lighting_render_data; }
		void setSkyboxRenderData(const std::shared_ptr<SkyboxRenderData>& skybox_render_data) { m_skybox_render_data = skybox_render_data; }
		void setBillboardRenderDatas(const std::vector<std::shared_ptr<BillboardRenderData>>& billboard_render_datas) {
			m_billboard_render_datas = billboard_render_datas;
		}
		void setTransparencyRenderDatas(const std::vector<std::shared_ptr<RenderData>>& transparency_render_datas) {
			m_transparency_render_datas = transparency_render_datas;
		}

		VkImageView getColorImageView() { return m_color_image_view.view; }

	private:
		enum class ERendererType
		{
			Deferred, Forward
		};

		void render_mesh(const std::shared_ptr<RenderData>& render_data, ERendererType renderer_type);

		std::vector<VkFormat> m_formats;

		// color attachment
		VmaImageView m_color_image_view;

		// gbuffer attachment
		VmaImageViewSampler m_normal_texture_sampler;
		VmaImageViewSampler m_base_color_texture_sampler;
		VmaImageViewSampler m_emissive_texture_sampler;
		VmaImageViewSampler m_metallic_roughness_occlusion_texture_sampler;

		// depth stencil attachment
		VmaImageViewSampler m_depth_stencil_texture_sampler;

		// extra render data
		std::vector<std::shared_ptr<RenderData>> m_transparency_render_datas;
		std::shared_ptr<LightingRenderData> m_lighting_render_data;
		std::shared_ptr<SkyboxRenderData> m_skybox_render_data;
		std::vector<std::shared_ptr<BillboardRenderData>> m_billboard_render_datas;
	};
}