#include "property_ui.h"

namespace Bamboo
{

	void PropertyUI::init()
	{
		m_title = "Property";
	}

	void PropertyUI::construct()
	{
		EditorUI::construct();

		ImGui::Begin(combine(ICON_FA_STREAM, m_title).c_str());

		for (int i = 0; i < 10; ++i)
		{
			ImGui::Text((m_title + std::to_string(i)).c_str());
		}

		ImGui::End();
	}

	void PropertyUI::destroy()
	{
		EditorUI::destroy();

	}

}