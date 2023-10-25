#pragma once

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <imgui/imgui.h>

namespace Bamboo
{
	struct FolderNode
	{
		std::string name;
		std::string dir;
		std::vector<std::string> child_files;
		std::vector<uint32_t> child_folders;

		bool is_root;
		bool is_leaf;
	};

	struct HoverState
	{
		bool is_hovered;
		ImVec2 rect_min;
		ImVec2 rect_max;
	};

	class IFolderTreeUI
	{
	protected:
		void pollFolders();
		void constructFolderTree();

		virtual void openFolder(std::string folder);
		bool createFolder();
		bool deleteFolder();

		std::string m_selected_folder;
		std::vector<FolderNode> m_folder_nodes;
		bool show_engine_assets = false;
		bool is_folder_tree_hovered = false;

	private:
		void constructFolderTree(const std::vector<FolderNode>& folder_nodes, uint32_t index);

		std::map<std::string, bool> m_folder_opened_map;
	};
}