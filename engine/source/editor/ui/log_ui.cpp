#include "log_ui.h"

namespace Bamboo
{

	void LogUI::init()
	{
		m_title = "Log";
	}

	void LogUI::construct()
	{
		EditorUI::construct();

		ImGui::Begin(m_title.c_str());

		for (int i = 0; i < 10; ++i)
		{
			ImGui::Text((m_title + std::to_string(i)).c_str());
		}

		ImGui::End();
	}

	void LogUI::destroy()
	{

	}

}