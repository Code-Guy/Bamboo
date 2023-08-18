#pragma once

#include "render_pass.h"

namespace Bamboo
{
	enum class EFilterType
	{
		Irradiance, Prefilter
	};

	class FilterCubePass : public RenderPass
	{
	public:
		FilterCubePass(std::shared_ptr<class TextureCube>& skybox_texture_cube);

		virtual void init() override;
		virtual void render() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		VmaImageViewSampler getIrradianceTextureSampler() { return m_cube_image_view_samplers[0]; }
		VmaImageViewSampler getPrefilterTextureSampler() { return m_cube_image_view_samplers[1]; }
		uint32_t getPrefilterMipLevels() { return m_mip_levels[1]; }

	private:
		VkFormat m_formats[2];
		uint32_t m_sizes[2];
		uint32_t m_mip_levels[2];

		VkRenderPass m_render_passes[2];
		VkFramebuffer m_framebuffers[2];
		VmaImageView m_color_image_views[2];
		VmaImageViewSampler m_cube_image_view_samplers[2];
		std::shared_ptr<class TextureCube> m_skybox_texture_cube;
		std::shared_ptr<class StaticMesh> m_skybox_mesh;
	};
}