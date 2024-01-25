#include "menu_ui.h"
#include "engine/core/base/macro.h"
#include "engine/core/config/config_manager.h"
#include "engine/core/event/event_system.h"
#include "engine/resource/asset/asset_manager.h"
#include "engine/function/framework/world/world_manager.h"

#include <imgui/imgui_internal.h>

namespace Bamboo
{

	void MenuUI::init()
	{
		m_title = "Menu";

		// load editor layout
		std::string layout_name = g_engine.configManager()->getEditorLayout();
		m_layout_path = g_engine.fileSystem()->absolute("asset/engine/layout/" + layout_name);
		ImGui::LoadIniSettingsFromDisk(m_layout_path.c_str());

		m_template_worlds = {
			{ "empty", "asset/engine/world/empty.world", loadImGuiImageFromFile("asset/engine/world/empty.png")},
			{ "basic", "asset/engine/world/basic.world", loadImGuiImageFromFile("asset/engine/world/basic.png")}
		};
		for (const auto& template_world : m_template_worlds)
		{
			m_template_world_hover_states[template_world.name] = { false };
		}
	}

	void MenuUI::construct()
	{
		// poll shortcuts states
		pollShortcuts();

		// set menu background color to opaque
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				constructFileMenu();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				constructEditMenu();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				constructViewMenu();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				constructHelpMenu();
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		// construct popups
		static char world_name[128];
		const float k_spacing = sScalar(4);
		if (m_showing_new_world_popup)
		{
			ImGui::SetNextWindowSize(sImVec2(420, 500));
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			ImGui::OpenPopup("New World");
			if (ImGui::BeginPopupModal("New World", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
			{
				const float k_middle_height = sScalar(55.0f);
				const float k_bottom_height = sScalar(25.0f);
				
				ImVec2 content_size = ImGui::GetContentRegionAvail();

				float top_height = content_size.y - k_middle_height - k_bottom_height - k_spacing * 2;
				ImGui::BeginChild("new_world_top", ImVec2(content_size.x, top_height));

				const float k_folder_tree_width_scale = 0.5f;
				ImGui::BeginChild("template_world", ImVec2(content_size.x * k_folder_tree_width_scale - k_spacing, top_height), true);
				constructTemplateWorldPanel();
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("folder_tree", ImVec2(content_size.x * (1.0 - k_folder_tree_width_scale) - k_spacing, top_height), true);
				constructFolderTree();
				ImGui::EndChild();
				ImGui::EndChild();

				ImGui::BeginChild("new_world_middle", ImVec2(content_size.x, k_middle_height), true);
				ImGui::Text("path:");
				ImGui::SameLine();
				ImGui::Text(m_selected_folder.c_str());

				ImGui::Text("name:");
				ImGui::SameLine();
				ImGui::InputText("##world_name", world_name, IM_ARRAYSIZE(world_name));
				ImGui::EndChild();
				
				ImGui::BeginChild("new_world_bottom", ImVec2(content_size.x, k_bottom_height), false);
				float button_width = sScalar(60.0f);
				float button_offset_x = (ImGui::GetContentRegionAvail().x - button_width * 2 - k_spacing) / 2.0f;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_offset_x);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
				if (ImGui::Button("create", ImVec2(button_width, 0)))
				{
					std::string world_name_str = world_name;
					if (!m_selected_folder.empty() && !world_name_str.empty())
					{
						const std::string& template_url = m_template_worlds[m_selected_template_world_index].url;
						std::string save_as_url = m_selected_folder + "/" + world_name_str + ".world";

						clearEntitySelection();
						g_engine.worldManager()->createWorld(template_url, save_as_url);

						m_showing_new_world_popup = false;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("cancel", ImVec2(button_width, 0)))
				{
					m_showing_new_world_popup = false;
				}
				ImGui::EndChild();

				ImGui::EndPopup();
			}
		}

		if (m_showing_open_world_popup)
		{
			ImGui::SetNextWindowSize(sImVec2(300, 500));
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			ImGui::OpenPopup("Open World");
			if (ImGui::BeginPopupModal("Open World", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
			{
				for (const std::string& current_world_url : m_current_world_urls)
				{
					ImGuiTreeNodeFlags tree_node_flags = 0;
					tree_node_flags |= ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick;
					if (current_world_url == m_selected_world_url)
					{
						tree_node_flags |= ImGuiTreeNodeFlags_Selected;
					}

					if (ImGui::TreeNodeEx(current_world_url.c_str(), tree_node_flags))
					{
						ImGui::TreePop();
					}

					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
					{
						m_selected_world_url = current_world_url;
					}
				}

				float button_width = sScalar(60.0f);
				float button_offset_x = (ImGui::GetContentRegionAvail().x - button_width * 2 - k_spacing) / 2.0f;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_offset_x);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - 30);
				if (ImGui::Button("open", ImVec2(button_width, 0)))
				{
					clearEntitySelection();
					g_engine.worldManager()->openWorld(m_selected_world_url);
					m_showing_open_world_popup = false;
				}

				ImGui::SameLine();
				if (ImGui::Button("cancel", ImVec2(button_width, 0)))
				{
					m_showing_open_world_popup = false;
				}

				ImGui::EndPopup();
			}
		}

		if (m_showing_save_as_world_popup)
		{
			ImGui::SetNextWindowSize(sImVec2(300, 500));
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

			ImGui::OpenPopup("Save As World");
			if (ImGui::BeginPopupModal("Save As World", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
			{
				const float k_middle_height = sScalar(55.0f);
				const float k_bottom_height = sScalar(25.0f);

				ImVec2 content_size = ImGui::GetContentRegionAvail();

				float top_height = content_size.y - k_middle_height - k_bottom_height - k_spacing * 2;
				ImGui::BeginChild("new_world_top", ImVec2(content_size.x, top_height), true);
				constructFolderTree();
				ImGui::EndChild();

				ImGui::BeginChild("new_world_middle", ImVec2(content_size.x, k_middle_height), true);
				ImGui::Text("path:");
				ImGui::SameLine();
				ImGui::Text(m_selected_folder.c_str());

				ImGui::Text("name:");
				ImGui::SameLine();
				ImGui::InputText("##world_name", world_name, IM_ARRAYSIZE(world_name));
				ImGui::EndChild();

				ImGui::BeginChild("new_world_bottom", ImVec2(content_size.x, k_bottom_height), false);
				float button_width = sScalar(60.0f);
				float button_offset_x = (ImGui::GetContentRegionAvail().x - button_width * 2 - k_spacing) / 2.0f;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_offset_x);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
				if (ImGui::Button("save", ImVec2(button_width, 0)))
				{
					std::string world_name_str = world_name;
					if (!m_selected_folder.empty() && !world_name_str.empty())
					{
						std::string url = m_selected_folder + "/" + world_name_str + ".world";
						g_engine.worldManager()->saveAsWorld(url);

						m_showing_save_as_world_popup = false;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("cancel", ImVec2(button_width, 0)))
				{
					m_showing_save_as_world_popup = false;
				}
				ImGui::EndChild();

				ImGui::EndPopup();
			}
		}
	}

	void MenuUI::destroy()
	{
		EditorUI::destroy();

		if (g_engine.configManager()->getSaveLayout())
		{
			ImGui::SaveIniSettingsToDisk(m_layout_path.c_str());
		}
	}

	void MenuUI::constructFileMenu()
	{
		if (ImGui::MenuItem("New", "Ctrl+N"))
		{
			newWorld();
		}

		if (ImGui::MenuItem("Open", "Ctrl+O"))
		{
			openWorld();
		}

		if (ImGui::BeginMenu("Open Recent"))
		{
			ImGui::MenuItem("a.world");
			ImGui::MenuItem("b.world");
			ImGui::MenuItem("c.world");
			if (ImGui::BeginMenu("More.."))
			{
				ImGui::MenuItem("d.world");
				ImGui::MenuItem("e.world");
				ImGui::MenuItem("f.world");
				ImGui::MenuItem("g.world");
				ImGui::MenuItem("h.world");
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Save", "Ctrl+S"))
		{
			saveWorld();
		}

		if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S"))
		{
			saveAsWorld();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Quit", "Alt+F4")) 
		{
			quit();
		}
	}

	void MenuUI::constructEditMenu()
	{
		if (ImGui::MenuItem("Undo", "Ctrl+Z"))
		{
			undo();
		}

		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false))
		{
			redo();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Cut", "Ctrl+X"))
		{
			cut();
		}

		if (ImGui::MenuItem("Copy", "Ctrl+C"))
		{
			copy();
		}

		if (ImGui::MenuItem("Paste", "Ctrl+V"))
		{
			paste();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Editor Settings", "Ctrl+E"))
		{
			editorSettings();
		}

		if (ImGui::MenuItem("Project Settings", "Ctrl+P"))
		{
			projectSettings();
		}
	}

	void MenuUI::constructViewMenu()
	{
		if (ImGui::MenuItem("Load Layout"))
		{
			
		}

		if (ImGui::MenuItem("Save Layout"))
		{
			
		}
	}

	void MenuUI::constructHelpMenu()
	{
		if (ImGui::MenuItem("Documents"))
		{

		}

		if (ImGui::MenuItem("About"))
		{

		}
	}

	void MenuUI::pollShortcuts()
	{
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, 0, ImGuiInputFlags_RouteAlways))
		{
			newWorld();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, 0, ImGuiInputFlags_RouteAlways))
		{
			openWorld();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, 0, ImGuiInputFlags_RouteAlways))
		{
			saveWorld();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N, 0, ImGuiInputFlags_RouteAlways))
		{
			saveAsWorld();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Q, 0, ImGuiInputFlags_RouteAlways))
		{
			quit();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, 0, ImGuiInputFlags_RouteAlways))
		{
			undo();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, 0, ImGuiInputFlags_RouteAlways))
		{
			redo();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_X, 0, ImGuiInputFlags_RouteAlways))
		{
			cut();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, 0, ImGuiInputFlags_RouteAlways))
		{
			copy();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, 0, ImGuiInputFlags_RouteAlways))
		{
			paste();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_E, 0, ImGuiInputFlags_RouteAlways))
		{
			editorSettings();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, 0, ImGuiInputFlags_RouteAlways))
		{
			projectSettings();
		}
	}

	void MenuUI::newWorld()
	{
		if (isPoppingUp())
		{
			return;
		}

		m_showing_new_world_popup = true;
	}

	void MenuUI::openWorld()
	{
		if (isPoppingUp())
		{
			return;
		}

		m_showing_open_world_popup = true;
		m_current_world_urls.clear();

		std::vector<std::string> all_files;
		FileWatcher::get().getAllFiles(all_files);
		for (const auto& file : all_files)
		{
			if (g_engine.assetManager()->getAssetType(file) == EAssetType::World)
			{
				std::string world_name = g_engine.fileSystem()->basename(file);
				std::string current_world_name = g_engine.worldManager()->getCurrentWorldName();
				if (world_name != current_world_name)
				{
					m_current_world_urls.push_back(g_engine.fileSystem()->relative(file));
				}
			}
		}
	}

	void MenuUI::saveWorld()
	{
		if (isPoppingUp())
		{
			return;
		}

		g_engine.worldManager()->saveWorld();
	}

	void MenuUI::saveAsWorld()
	{
		if (isPoppingUp())
		{
			return;
		}

		m_showing_save_as_world_popup = true;
	}

	void MenuUI::quit()
	{

	}

	void MenuUI::undo()
	{

	}

	void MenuUI::redo()
	{

	}

	void MenuUI::cut()
	{

	}

	void MenuUI::copy()
	{

	}

	void MenuUI::paste()
	{

	}

	void MenuUI::editorSettings()
	{

	}

	void MenuUI::projectSettings()
	{

	}

	void MenuUI::constructTemplateWorldPanel()
	{
		ImVec2 icon_size = sImVec2(80, 80);
		ImGuiStyle& style = ImGui::GetStyle();
		float max_pos_x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

		float clip_rect_min_y = ImGui::GetCursorScreenPos().y + ImGui::GetScrollY();
		float clip_rect_max_y = clip_rect_min_y + ImGui::GetContentRegionAvail().y;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, sImVec2(15.0f, 24.0f));
		for (size_t i = 0; i < m_template_worlds.size(); ++i)
		{
			const std::string& template_world_name = m_template_worlds[i].name;
			HoverState& hover_state = m_template_world_hover_states[template_world_name];

			ImGui::BeginGroup();

			// draw hovered/selected background rect
			bool is_hovered = hover_state.is_hovered;
			bool is_selected = m_selected_template_world_index == i;
			if (is_hovered || is_selected)
			{
				ImVec4 color = ImVec4(50, 50, 50, 255);
				if (!is_hovered && is_selected)
				{
					color = ImVec4(0, 112, 224, 255);
				}
				else if (is_hovered && is_selected)
				{
					color = ImVec4(14, 134, 255, 255);
				}

				ImDrawFlags draw_flags = ImDrawFlags_RoundCornersBottom;
				const float k_margin = sScalar(4);
				ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(hover_state.rect_min.x - k_margin, hover_state.rect_min.y - k_margin),
					ImVec2(hover_state.rect_max.x + k_margin, hover_state.rect_max.y + k_margin),
					IM_COL32(color.x, color.y, color.z, color.w), sScalar(3.0f), draw_flags);
			}

			// draw image
			ImGui::Image(m_template_worlds[i].icon->tex_id, icon_size);

			// draw asset name text
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - sScalar(20.0f));
			float text_width = ImGui::CalcTextSize(template_world_name.c_str()).x;
			if (text_width > icon_size.x)
			{
				ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + icon_size.x);
				ImGui::Text(template_world_name.c_str());
				ImGui::PopTextWrapPos();
			}
			else
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPos().x + (icon_size.x - text_width) * 0.5f);
				ImGui::Text(template_world_name.c_str());
			}

			ImGui::EndGroup();

			// update asset hover and selection status
			hover_state.is_hovered = ImGui::IsItemHovered();
			if (ImGui::IsItemClicked())
			{
				m_selected_template_world_index = i;
			}
			hover_state.rect_min = ImGui::GetItemRectMin();
			hover_state.rect_max = ImGui::GetItemRectMax();

			float current_pos_x = ImGui::GetItemRectMax().x;
			float next_pos_x = current_pos_x + style.ItemSpacing.x + icon_size.x;
			if (i < m_template_worlds.size() - 1 && next_pos_x < max_pos_x)
			{
				ImGui::SameLine();
			}
		}
		ImGui::PopStyleVar();
	}

	void MenuUI::constructWorldURLPanel()
	{

	}

	void MenuUI::clearEntitySelection()
	{
		g_engine.eventSystem()->syncDispatch(std::make_shared<SelectEntityEvent>(UINT_MAX));
	}

}