#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class DirectionalLightShadowPass : public RenderPass
	{
	public:
		DirectionalLightShadowPass();

		virtual void init() override;
		virtual void render() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

	private:
		void updateCascades();

		VkFormat m_format;
		uint32_t m_size;
		float m_cascade_split_lambda;

		VmaImageViewSampler m_depth_image_view_sampler;
		std::shared_ptr<DirectionalLightShadowPassRenderData> m_dlsp_render_data;

		ShadowCascadeUBO m_shadow_cascade_ubo;
		std::vector<VmaBuffer> m_shadow_cascade_ubs;
	};
}