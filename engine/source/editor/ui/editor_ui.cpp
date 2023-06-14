#include "editor_ui.h"

namespace Bamboo
{

	void EditorUI::construct()
	{
		handle_windows_resize();
	}

	bool EditorUI::handle_windows_resize()
	{
		ImVec2 m_new_size = ImGui::GetContentRegionAvail();
		uint32_t new_width = static_cast<uint32_t>(m_new_size.x);
		uint32_t new_height = static_cast<uint32_t>(m_new_size.y);

		if (m_width != new_width || m_height != new_height)
		{
			m_width = new_width;
			m_height = new_height;

			on_window_resize();
			return true;
		}

		return false;
	}

}