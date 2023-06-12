#pragma once

#include "runtime/core/vulkan/render_pass.h"

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

	private:
		
	};
}