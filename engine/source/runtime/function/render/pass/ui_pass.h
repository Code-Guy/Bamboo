#pragma once

#include "pass.h"

namespace Bamboo
{
	class UIPass : public Pass
	{
	public:
		virtual void init() override;
		virtual void render() override;
		virtual void destroy() override;
		virtual void createSwapchainObjects() override;
		virtual void destroySwapchainObjects() override;

	private:

	};
}