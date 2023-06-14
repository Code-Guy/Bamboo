#include "render_pass.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	void RenderPass::destroy()
	{
		vkDestroyRenderPass(VulkanRHI::get().getDevice(), m_render_pass, nullptr);
		vkDestroyDescriptorPool(VulkanRHI::get().getDevice(), m_descriptor_pool, nullptr);
		destroyResizableObjects();
	}

	void RenderPass::createResizableObjects(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
	}

	void RenderPass::on_resize(uint32_t width, uint32_t height)
	{
		destroyResizableObjects();
		createResizableObjects(width, height);
	}

	bool RenderPass::is_minimize()
	{
		return m_width == 0 || m_height == 0;
	}

}