#include "render_system.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/pass/ui_pass.h"
#include "runtime/function/render/pass/base_pass.h"

namespace Bamboo
{

	void RenderSystem::init()
	{
		std::shared_ptr<BasePass> base_pass = std::make_shared<BasePass>();
		base_pass->init();

		m_ui_pass = std::make_shared<UIPass>();
		m_ui_pass->init();

		VulkanRHI::get().getRenderPasses()[ERenderPassType::Base] = base_pass;
		VulkanRHI::get().getRenderPasses()[ERenderPassType::UI] = m_ui_pass;
	}

	void RenderSystem::tick(float delta_time)
	{
		VulkanRHI::get().render();
	}

	void RenderSystem::destroy()
	{

	}

	void RenderSystem::setConstructUIFunc(const std::function<void()>& construct_ui_func)
	{
		m_ui_pass->setConstructFunc(construct_ui_func);
	}

}