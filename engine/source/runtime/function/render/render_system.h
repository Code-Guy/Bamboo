#pragma once

#include <memory>
#include <functional>

namespace Bamboo
{
	class RenderSystem
	{
	public:
		void init();
		void tick(float delta_time);
		void destroy();

		void setConstructUIFunc(const std::function<void()>& construct_ui_func);

	private:
		std::shared_ptr<class UIPass> m_ui_pass;
	};
}