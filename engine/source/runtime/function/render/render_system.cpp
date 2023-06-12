#include "render_system.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/ui_pass.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		std::shared_ptr<UIPass> ui_pass = std::make_shared<UIPass>();
		ui_pass->init();
		VulkanRHI::instance().addRenderPass(ui_pass);
	}

	void RenderSystem::tick(float delta_time)
	{
		VulkanRHI::instance().render();
	}

	void RenderSystem::destroy()
	{

	}

}