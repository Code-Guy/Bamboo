#include "folder_tree_ui.h"
#include "engine/core/base/macro.h"
#include "engine/platform/timer/timer.h"

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/font/IconsFontAwesome5.h>
#include <queue>

namespace Bamboo
{

	void IFolderTreeUI::constructFolderTree()
	{
		constructFolderTree(FileWatcher::get().getRootFolderNode());
	}

	void IFolderTreeUI::constructFolderTree(std::shared_ptr<FolderNode>& folder_node)
	{
		if (!folder_node)
		{
			return;
		}

		// ignore internal engine folder if show engint assets option off
		if (!m_show_engine_assets && isEngineFolder(folder_node->dir))
		{
			return;
		}

		ImGuiTreeNodeFlags tree_node_flags = 0;
		tree_node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (folder_node->is_root)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}
		else if (folder_node->is_leaf)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (folder_node->dir == m_selected_folder)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Selected;
		}

		if (m_folder_opened_map.find(folder_node->name) == m_folder_opened_map.end())
		{
			m_folder_opened_map[folder_node->name] = false;
		}
		bool is_treenode_opened = ImGui::TreeNodeEx(folder_node->dir.c_str(), tree_node_flags, "%s %s", m_folder_opened_map[folder_node->name] ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, folder_node->name.c_str());
		m_folder_opened_map[folder_node->name] = is_treenode_opened && !folder_node->is_leaf;
		if ((ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) && !ImGui::IsItemToggledOpen())
		{
			openFolder(folder_node->dir);
		}

		if (ImGui::IsItemHovered())
		{
			m_is_folder_tree_hovered |= true;
		}

		if (is_treenode_opened)
		{
			if (!folder_node->child_folders.empty())
			{
				const float k_unindent_w = 8;
				ImGui::Unindent(k_unindent_w);

				for (std::shared_ptr<FolderNode> child_folder : folder_node->child_folders)
				{
					constructFolderTree(child_folder);
				}
			}

			ImGui::TreePop();
		}
	}

	void IFolderTreeUI::openFolder(std::string folder)
	{
		if (!folder.empty())
		{
			m_selected_folder = g_engine.fileSystem()->relative(folder);
		}
	}

	std::string IFolderTreeUI::createFolder()
	{
		std::string new_folder_prefix = m_selected_folder + "/new_folder";
		std::string new_folder_name = new_folder_prefix;

		int index = 1;
		while (!g_engine.fileSystem()->createDir(new_folder_name))
		{
			new_folder_name = new_folder_prefix + "_" + std::to_string(index++);
		}
		LOG_INFO("create folder: {}", new_folder_name);
		return new_folder_name;
	}

	bool IFolderTreeUI::deleteFolder(const std::string& folder_name)
	{
		LOG_INFO("delete folder: {}", folder_name);
		g_engine.fileSystem()->removeDir(folder_name, true);
		return true;
	}

	bool IFolderTreeUI::rename(const std::string& filename, const ImVec2& size)
	{
		std::string basename = g_engine.fileSystem()->basename(filename);
		std::string dir = g_engine.fileSystem()->dir(filename) + "/";
		ImGui::PushItemWidth(size.x);
		strcpy(m_new_name_buffer, basename.c_str());

		ImGui::SetKeyboardFocusHere();
		ImGui::InputText("##NewName", m_new_name_buffer, IM_ARRAYSIZE(m_new_name_buffer), ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopItemWidth();

		// if press enter or not focus, exit renaming status, else maintain renaming status
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) 
			|| (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
		{
			g_engine.fileSystem()->renameFile(dir, basename, m_new_name_buffer);
			return false;
		}
		return true;
	}

	bool IFolderTreeUI::isEngineFolder(const std::string& folder)
	{
		return folder.find("asset/engine") != std::string::npos;
	}

}