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

		void updateCube(const ShadowCubeCreateInfo& shadow_cube_ci);
		VmaImageViewSampler getShadowImageViewSampler() { return m_shadow_image_view_sampler; }

	private:
		std::vector<VkFormat> m_formats;
		uint32_t m_size;

		VmaImageViewSampler m_depth_image_view_sampler;
		VmaImageViewSampler m_shadow_image_view_sampler;
		std::vector<VmaBuffer> m_shadow_cube_ubs;

		glm::vec3 m_light_pos;
	};
}