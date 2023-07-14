#pragma once

#include "editor/base/editor_ui.h"
#include <map>

namespace Bamboo
{
	class MenuUI : public EditorUI
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

		void newProject();
		void openProject(); 
		void saveProject();
		void saveAsProject();
		void quit(); 

		void undo(); 
		void redo(); 
		void cut(); 
		void copy(); 
		void paste(); 

		void editorSettings(); 
		void projectSettings();

		std::string m_layout_path;
	};
}