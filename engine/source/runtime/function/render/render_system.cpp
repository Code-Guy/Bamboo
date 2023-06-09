#include "render_system.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		
	}

	void RenderSystem::tick(float delta_time)
	{
		VulkanRHI::instance().render();
	}

	void RenderSystem::destroy()
	{

	}

}