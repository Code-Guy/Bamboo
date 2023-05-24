#include "render_system.h"
#include "runtime/function/render/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		vulkan_rhi = std::make_shared<class VulkanRHI>();
		vulkan_rhi->init();
	}

	void RenderSystem::tick(float delta_time)
	{

	}

	void RenderSystem::destroy()
	{
		vulkan_rhi->destroy();
		vulkan_rhi.reset();
	}

}