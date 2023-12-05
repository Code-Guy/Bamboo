#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class PointLightShadowPass : public RenderPass
	{
	public:
		PointLightShadowPass();

		virtual void init() override;
		virtual void render() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		void updateCubes(const std::vector<ShadowCubeCreateInfo>& shadow_cube_cis);
		const std::vector<VmaImageViewSampler>& getShadowImageViewSamplers();

	private:
		void createDynamicBuffers(size_t size);

		std::vector<VkFormat> m_formats;
		uint32_t m_size;

		VmaImageViewSampler m_depth_image_view_sampler;

		std::vector<VmaImageViewSampler> m_shadow_image_view_samplers;
		std::vector<VkFramebuffer> m_framebuffers;
		std::vector<std::vector<VmaBuffer>> m_shadow_cube_ubss;

		uint32_t m_light_num;
		std::vector<glm::vec3> m_light_poss;
	};
}