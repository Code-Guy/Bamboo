#include "world_ui.h"

namespace Bamboo
{

	void WorldUI::construct()
	{
		m_title = "World";

		ImGui::Begin(m_title.c_str());

		for (int i = 0; i < 10; ++i)
		{
			ImGui::Text((m_title + std::to_string(i)).c_str());
		}
		
		ImGui::End();
	}

}