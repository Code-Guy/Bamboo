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

		void updateCascades(const ShadowCascadeCreateInfo& shadow_cascade_ci);
		VmaImageViewSampler getShadowImageViewSampler() { return m_shadow_image_view_sampler; }

		ShadowCascadeUBO m_shadow_cascade_ubo;
		float m_cascade_splits[SHADOW_CASCADE_NUM];

	private:
		VkFormat m_format;
		uint32_t m_size;
		float m_cascade_split_lambda;

		VmaImageViewSampler m_shadow_image_view_sampler;
		std::vector<VmaBuffer> m_shadow_cascade_ubs;
	};
}