#include "menu_ui.h"
#include "runtime/core/base/macro.h"
#include <imgui/imgui_internal.h>

namespace Bamboo
{

	void MenuUI::init()
	{
		m_title = "Menu";
	}

	void MenuUI::construct()
	{
		EditorUI::construct();

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
	}

	void MenuUI::destroy()
	{

	}

	void MenuUI::constructFileMenu()
	{
		if (ImGui::MenuItem("New", "Ctrl+N"))
		{
			newProject();
		}

		if (ImGui::MenuItem("Open", "Ctrl+O"))
		{
			openProject();
		}

		if (ImGui::BeginMenu("Open Recent"))
		{
			ImGui::MenuItem("a.bproject");
			ImGui::MenuItem("b.bproject");
			ImGui::MenuItem("c.bproject");
			if (ImGui::BeginMenu("More.."))
			{
				ImGui::MenuItem("d.bproject");
				ImGui::MenuItem("e.bproject");
				ImGui::MenuItem("f.bproject");
				ImGui::MenuItem("g.bproject");
				ImGui::MenuItem("h.bproject");
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Save", "Ctrl+S"))
		{
			saveProject();
		}

		if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S"))
		{
			saveAsProject();
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
			newProject();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, 0, ImGuiInputFlags_RouteAlways))
		{
			openProject();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, 0, ImGuiInputFlags_RouteAlways))
		{
			saveProject();
		}
		else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N, 0, ImGuiInputFlags_RouteAlways))
		{
			saveAsProject();
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

	void MenuUI::newProject()
	{
		LOG_INFO("new project");
	}

	void MenuUI::openProject()
	{

	}

	void MenuUI::saveProject()
	{

	}

	void MenuUI::saveAsProject()
	{

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

}