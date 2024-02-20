#include "tool_ui.h"
#include "engine/core/base/macro.h"
#include "engine/function/framework/world/world_manager.h"
#include "engine/function/physics/physics_system.h"
#include <map>

namespace Bamboo
{
	void ToolUI::init()
	{
		m_title = "Tool";

		entity_categories = {
			std::string(ICON_FA_CUBE) + " Entities",
			std::string(ICON_FA_LIGHTBULB) + " Lights",
			std::string(ICON_FA_SHAPES) + " Primitives"
		};

		entity_typess = {
			{ std::string(ICON_FA_ASTERISK) + " Empty Entity" },
			{ std::string(ICON_FA_SUN) + " Directional Light",
			  std::string(ICON_FA_CLOUD_MEATBALL) + " Sky Light",
			  std::string(ICON_FA_LIGHTBULB) + " Point Light",
			  std::string(ICON_FA_FLASH_LIGHT) + " Spot Light"},
			{
				std::string(ICON_FA_STAR) + " Cube",
				std::string(ICON_FA_STAR) + " Sphere",
				std::string(ICON_FA_STAR) + " Cylinder",
				std::string(ICON_FA_STAR) + " Cone",
				std::string(ICON_FA_STAR) + " Plane",
			}
		};

		const auto& entity_class_names = g_engine.worldManager()->getCurrentWorld()->getEntityClassNames();
		for (const std::string& entity_class_name : entity_class_names)
		{
			entity_typess.front().push_back(std::string(ICON_FA_ASTERISK) + " " + entity_class_name);
		}
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
		const auto& wm = g_engine.worldManager();

		// save
		const float kButtonHeight = 30.0f;
		const ImVec2 kButtonSize = sImVec2(kButtonHeight, kButtonHeight);
		ImGui::PushFont(bigIconFont());
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
		if (ImGui::Button(ICON_FA_SAVE, kButtonSize))
		{
			wm->saveWorld();
		}
		ImGui::PopFont();

		// create entities
		ImGui::SameLine();
		sprintf(m_title_buf, "Create %s", ICON_FA_CHEVRON_DOWN);
		if (ImGui::Button(m_title_buf, sImVec2(80, kButtonHeight)))
		{
			ImGui::OpenPopup("create_entity");
		}
		constructCreateEntityPopup();

		// play/step/stop
		ImGui::SameLine();
		ImGui::SetCursorPosX(region_w * 0.5f - kButtonSize.x * 1.5f - spacing.x);

		EWorldMode current_world_mode = wm->getWorldMode();
		const char* play_icon_text = current_world_mode == EWorldMode::Edit ? ICON_FA_PLAY :
			(current_world_mode == EWorldMode::Play ? ICON_FA_PAUSE : ICON_FA_FORWARD);
		if (ImGui::Button(play_icon_text, kButtonSize))
		{
			switch (current_world_mode)
			{
			case EWorldMode::Edit:
				wm->setWorldMode(EWorldMode::Play);
				break;
			case EWorldMode::Play:
				wm->setWorldMode(EWorldMode::Pause);
				break;
			case EWorldMode::Pause:
				wm->setWorldMode(EWorldMode::Play);
				break;
			default:
				break;
			}
		}

		ImGui::SameLine();
		if (current_world_mode != EWorldMode::Pause)
		{
			ImGui::BeginDisabled(true);
		}
		if (ImGui::Button(ICON_FA_STEP_FORWARD, kButtonSize))
		{
			g_engine.physicsSystem()->step();
			wm->getCurrentWorld()->step();
		}
		if (current_world_mode != EWorldMode::Pause)
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		if (current_world_mode == EWorldMode::Edit)
		{
			ImGui::BeginDisabled(true);
		}
		if (ImGui::Button(ICON_FA_STOP, kButtonSize))
		{
			wm->setWorldMode(EWorldMode::Edit);
		}
		if (current_world_mode == EWorldMode::Edit)
		{
			ImGui::EndDisabled();
		}

		ImGui::PopStyleColor();
		ImGui::End();
	}

	void ToolUI::constructCreateEntityPopup()
	{
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
		if (ImGui::BeginPopup("create_entity"))
		{
			for (size_t i = 0; i < entity_categories.size(); ++i)
			{
				if (ImGui::BeginMenu(entity_categories[i].c_str()))
				{
					for (size_t j = 0; j < entity_typess[i].size(); ++j)
					{
						const std::string& entity_type = entity_typess[i][j];
						ImGui::Text(entity_type.c_str());
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoPreviewTooltip))
						{
							std::string playload_str = std::string(entity_categories[i].begin() + 4, entity_categories[i].end()) + "-" +
								std::string(entity_type.begin() + 4, entity_type.end());
							ImGui::SetDragDropPayload("create_entity", playload_str.data(), playload_str.size());
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