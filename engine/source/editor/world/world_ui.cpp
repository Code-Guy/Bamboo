#include "world_ui.h"

namespace Bamboo
{

	void WorldUI::init()
	{
		m_title = "World";
	}

	void WorldUI::construct()
	{
		EditorUI::construct();

		ImGui::Begin(combine(ICON_FA_GLOBE, m_title).c_str());

		for (int i = 0; i < 10; ++i)
		{
			ImGui::Text((m_title + std::to_string(i)).c_str());
		}
		
		ImGui::End();
	}

	void WorldUI::destroy()
	{
		EditorUI::destroy();

	}

}