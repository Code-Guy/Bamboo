#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class SpotLightShadowPass : public RenderPass
	{
	public:
		SpotLightShadowPass();

		virtual void init() override;
		virtual void render() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override {}
		virtual void destroyResizableObjects() override;

		void updateFrustums(const std::vector<ShadowFrustumCreateInfo>& shadow_frustum_cis);
		const std::vector<VmaImageViewSampler>& getShadowImageViewSamplers();

		std::vector<glm::mat4> m_light_view_projs;

	private:
		void createDynamicBuffers(size_t size);

		VkFormat m_format;
		uint32_t m_size;

		std::vector<VmaImageViewSampler> m_shadow_image_view_samplers;
		std::vector<VkFramebuffer> m_framebuffers;
	};
}