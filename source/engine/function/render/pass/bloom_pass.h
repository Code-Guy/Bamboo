#pragma once

#include "render_pass.h"

namespace Bamboo
{
	class BloomPass : public RenderPass
	{
	public:
		BloomPass();

		virtual void init() override;
		virtual void render() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorSetLayouts() override;
		virtual void createPipelineLayouts() override;
		virtual void createPipelines() override;
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

		const VmaImageViewSampler* getBloomFxTexture() { return &m_color_texture_sampler; }

	private:
		VkFormat m_format;
		VmaImageViewSampler m_blurred_texture_sampler;
		VmaImageViewSampler m_color_texture_sampler;

		struct
		{
			VkRenderPass render_pass;
			std::vector<VkPipeline> pipelines;
			std::vector<VkDescriptorSetLayout> desc_set_layouts;
			std::vector<VkPushConstantRange> push_constant_ranges;
			std::vector<VkPipelineLayout> pipeline_layouts;

			VmaImageViewSampler brightness_texture_sampler;
			VkFramebuffer frame_buffer{ VK_NULL_HANDLE };
		} m_brightness_pass;

		struct  
		{
			VkRenderPass render_pass;
			std::vector<VkPipeline> pipelines;
			std::vector<VkDescriptorSetLayout> desc_set_layouts;
			std::vector<VkPushConstantRange> push_constant_ranges;
			std::vector<VkPipelineLayout> pipeline_layouts;

			VmaImageViewSampler vert_blurred_texture_sampler;
			VkFramebuffer frame_buffer{ VK_NULL_HANDLE };
		} m_vert_blurred_pass;
	};
}