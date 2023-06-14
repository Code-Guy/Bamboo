#include "asset_ui.h"

namespace Bamboo
{

	void AssetUI::init()
	{
		m_title = "Asset";
	}

	void AssetUI::construct()
	{
		EditorUI::construct();

		ImGui::Begin(m_title.c_str());

		for (int i = 0; i < 10; ++i)
		{
			ImGui::Text((m_title + std::to_string(i)).c_str());
		}

		ImGui::End();
	}

	void AssetUI::destroy()
	{

	}

}