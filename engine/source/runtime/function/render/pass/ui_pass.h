#pragma once

#include "runtime/core/vulkan/render_pass.h"
#include <functional>

namespace Bamboo
{
	class UIPass : public RenderPass
	{
	public:
		virtual void init() override;
		virtual void prepare() override;
		virtual void record() override;
		virtual void destroy() override;
		virtual void createSwapchainObjects() override;
		virtual void destroySwapchainObjects() override;

		void setConstructFunc(const std::function<void()>& construct_func) { m_construct_func = construct_func; }

	private:
		std::function<void()> m_construct_func;
	};
}