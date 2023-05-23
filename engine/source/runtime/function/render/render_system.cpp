#include "render_system.h"
#include "runtime/function/render/vulkan/vulkan_device.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		vk_device = std::make_shared<class VulkanDevice>();
		vk_device->init();
	}

	void RenderSystem::tick(float delta_time)
	{

	}

	void RenderSystem::destroy()
	{
		vk_device->destroy();
		vk_device.reset();
	}

}