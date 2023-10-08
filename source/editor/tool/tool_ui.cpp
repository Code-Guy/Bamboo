#include "tool_ui.h"
#include "engine/core/base/macro.h"
#include "engine/function/framework/world/world_manager.h"
#include <map>

namespace Bamboo
{
	void ToolUI::init()
	{
		m_title = "Tool";
	}

	void ToolUI::construct()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
		sprintf(m_title_buf, "%s %s###%s", ICON_FA_TOOLS, m_title.c_str(), m_title.c_str());
		if (!ImGui::Begin(m_title_buf, nullptr, window_flags))
		{
			ImGui::End();
			return;
		}

		float region_w = ImGui::GetContentRegionAvail().x;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		// save
		const float kButtonHeight = 30.0f;
		const ImVec2 kButtonSize = ImVec2(kButtonHeight, kButtonHeight);
		ImGui::PushFont(bigIconFont());
		if (ImGui::Button(ICON_FA_SAVE, kButtonSize))
		{
			g_engine.worldManager()->saveWorld();
		}
		ImGui::PopFont();

		// create entities
		ImGui::SameLine();
		sprintf(m_title_buf, "create %s", ICON_FA_CHEVRON_DOWN);
		if (ImGui::Button(m_title_buf, ImVec2(80, kButtonHeight)))
		{
			ImGui::OpenPopup("create_entity");
		}
		constructCreateEntityPopup();

		// play/step/stop
		ImGui::SameLine();
		ImGui::SetCursorPosX(region_w * 0.5f - kButtonSize.x * 1.5f - spacing.x);
		if (ImGui::Button(ICON_FA_PLAY, kButtonSize))
		{

		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STEP_FORWARD, kButtonSize))
		{

		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP, kButtonSize))
		{

		}

		ImGui::End();
	}

	void ToolUI::constructCreateEntityPopup()
	{
		static const std::vector<std::string> categories = {
			std::string(ICON_FA_CUBE) + " basics", 
			std::string(ICON_FA_LIGHTBULB) + " lights", 
			std::string(ICON_FA_SHAPES) + " shapes"
		};

		static const std::vector<std::vector<std::string>> entity_typess = {
			{ std::string(ICON_FA_ASTERISK) + " empty entity" },
			{ std::string(ICON_FA_SUN) + " directional light", 
			  std::string(ICON_FA_CLOUD_MEATBALL) + " sky light", 
			  std::string(ICON_FA_LIGHTBULB) + " point light", 
			  std::string(ICON_FA_FLASH_LIGHT) + " spot light"},
			{
				std::string(ICON_FA_STAR) + " cube",
				std::string(ICON_FA_STAR) + " sphere",
				std::string(ICON_FA_STAR) + " cylinder",
				std::string(ICON_FA_STAR) + " cone",
				std::string(ICON_FA_STAR) + " plane",
			}
		};

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		if (ImGui::BeginPopup("create_entity"))
		{
			for (size_t i = 0; i < categories.size(); ++i)
			{
				if (ImGui::BeginMenu(categories[i].c_str()))
				{
					for (size_t j = 0; j < entity_typess[i].size(); ++j)
					{
						const std::string& entity_type = entity_typess[i][j];
						ImGui::Text(entity_type.c_str());
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoPreviewTooltip))
						{
							std::string trimmed_entity_type(entity_type.begin() + 4, entity_type.end());
							ImGui::SetDragDropPayload("create_entity", trimmed_entity_type.data(), trimmed_entity_type.size());
							ImGui::EndDragDropSource();
						}

						if (j != entity_typess[i].size() - 1)
						{
							ImGui::Separator();
						}
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndPopup();
		}
	}

}