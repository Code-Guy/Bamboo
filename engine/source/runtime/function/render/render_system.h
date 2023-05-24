#pragma once

#include <memory>

namespace Bamboo
{
	class RenderSystem
	{
	public:
		void init();
		void tick(float delta_time);
		void destroy();

	private:
		std::shared_ptr<class VulkanRHI> vulkan_rhi;
	};
}