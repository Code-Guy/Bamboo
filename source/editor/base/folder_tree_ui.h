#pragma once

#include <imgui/imgui.h>
#include "file_watcher.h"

namespace Bamboo
{
	struct HoverState
	{
		bool is_hovered;
		ImVec2 rect_min;
		ImVec2 rect_max;
	};

	class IFolderTreeUI
	{
	protected:
		void constructFolderTree();

		virtual void openFolder(std::string folder);
		std::string createFolder();
		bool deleteFolder(const std::string& folder_name);
		bool rename(const std::string& filename, const ImVec2& size = { 0.0f, 0.0f });
		bool isEngineFolder(const std::string& folder);

		std::string m_selected_folder;
		bool m_show_engine_assets = false;
		bool m_is_folder_tree_hovered = false;
		bool m_is_renaming = false;
		 
		char m_new_name_buffer[64] = "";

	private:
		void constructFolderTree(std::shared_ptr<FolderNode>& folder_node);

		std::map<std::string, bool> m_folder_opened_map;
	};
}