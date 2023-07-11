#include "property_ui.h"

namespace Bamboo
{

	void PropertyUI::init()
	{
		m_title = "Property";
	}

	void PropertyUI::construct()
	{
		sprintf(m_title_buf, "%s %s###%s", ICON_FA_STREAM, m_title.c_str(), m_title.c_str());
		if (!ImGui::Begin(m_title_buf))
		{
			ImGui::End();
			return;
		}

		ImGui::End();
	}

	void PropertyUI::destroy()
	{
		EditorUI::destroy();

	}

}