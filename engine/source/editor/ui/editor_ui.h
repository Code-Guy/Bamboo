#pragma once

#include <string>
#include <memory>
#include <imgui/imgui.h>

namespace Bamboo
{
	class EditorUI
	{
	public:
		virtual void init() = 0;
		virtual void construct();
		virtual void destroy() = 0;
		virtual void on_window_resize() {}

	protected:
		bool handle_windows_resize();

		std::string m_title;
		uint32_t m_width = 0, m_height = 0;

	private:
		
	};
}