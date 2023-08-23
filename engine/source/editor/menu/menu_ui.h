#pragma once

#include "editor/base/editor_ui.h"
#include "editor/base/folder_tree_ui.h"

namespace Bamboo
{
	class MenuUI : public EditorUI, public IFolderTreeUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:
		void constructFileMenu();
		void constructEditMenu();
		void constructViewMenu();
		void constructHelpMenu();

		void pollShortcuts();

		void newWorld();
		void openWorld(); 
		void saveWorld();
		void saveAsWorld();
		void quit(); 

		void undo(); 
		void redo(); 
		void cut(); 
		void copy(); 
		void paste(); 

		void editorSettings(); 
		void projectSettings();

		void constructTemplateWorldPanel();
		void constructWorldURLPanel();

		std::string m_layout_path;

		bool showing_new_world_popup = false;
		bool showing_open_world_popup = false;
		bool showing_save_as_world_popup = false;

		// new world
		struct TemplateWorld
		{
			std::string name;
			std::string url;
			std::shared_ptr<ImGuiImage> icon;
		};
		std::vector<TemplateWorld> m_template_worlds;
		std::map<std::string, HoverState> m_template_world_hover_states;
		uint32_t m_selected_template_world_index;

		// open world
		std::vector<std::string> m_current_world_urls;
		std::string m_selected_world_url;
	};
}