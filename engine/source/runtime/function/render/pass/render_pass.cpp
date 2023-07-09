#include "render_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	void RenderPass::destroy()
	{
		vkDestroyRenderPass(VulkanRHI::get().getDevice(), m_render_pass, nullptr);
		vkDestroyDescriptorPool(VulkanRHI::get().getDevice(), m_descriptor_pool, nullptr);
		for (VkDescriptorSetLayout desc_set_layout : m_desc_set_layouts)
		{
			vkDestroyDescriptorSetLayout(VulkanRHI::get().getDevice(), desc_set_layout, nullptr);
		}
		for (VkPipelineLayout pipeline_layout : m_pipeline_layouts)
		{
			vkDestroyPipelineLayout(VulkanRHI::get().getDevice(), pipeline_layout, nullptr);
		}
		vkDestroyPipelineCache(VulkanRHI::get().getDevice(), m_pipeline_cache, nullptr);
		for (VkPipeline pipeline : m_pipelines)
		{
			vkDestroyPipeline(VulkanRHI::get().getDevice(), pipeline, nullptr);
		}

		destroyResizableObjects();
	}

	void RenderPass::createResizableObjects(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
	}

	void RenderPass::onResize(uint32_t width, uint32_t height)
	{
		// ensure all device operations have done
		VulkanRHI::get().waitDeviceIdle();

		destroyResizableObjects();
		createResizableObjects(width, height);
	}

	bool RenderPass::isMinimize()
	{
		return m_width == 0 || m_height == 0;
	}

}