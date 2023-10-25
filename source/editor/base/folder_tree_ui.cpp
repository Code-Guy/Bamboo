#include "folder_tree_ui.h"
#include "engine/core/base/macro.h"
#include "engine/resource/asset/asset_manager.h"

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/font/IconsFontAwesome5.h>
#include <queue>

namespace Bamboo
{

	void IFolderTreeUI::pollFolders()
	{
		// recursively traverse the root folder by a queue
		const auto& fs = g_engine.fileSystem();

		m_folder_nodes.clear();
		std::queue<std::string> folder_queue;
		folder_queue.push(fs->getAssetDir());
		while (!folder_queue.empty())
		{
			size_t offset = m_folder_nodes.size();
			std::vector<std::string> folders;
			while (!folder_queue.empty())
			{
				m_folder_nodes.push_back({});
				folders.push_back(folder_queue.front());
				folder_queue.pop();
			}

			uint32_t child_offset = m_folder_nodes.size();
			for (size_t i = 0; i < folders.size(); ++i)
			{
				FolderNode& folder_node = m_folder_nodes[offset + i];
				folder_node.dir = folders[i];
				folder_node.name = fs->basename(folders[i]);
				folder_node.is_root = folders[i] == fs->getAssetDir();
				for (auto& file : std::filesystem::directory_iterator(folders[i]))
				{
					std::string filename = file.path().string();
					if (file.is_regular_file())
					{
						// don't show invalid asset type
						if (g_engine.assetManager()->getAssetType(filename) != EAssetType::Invalid)
						{
							folder_node.child_files.push_back(filename);
						}
					}
					else if (file.is_directory())
					{
						// ignore internal engine folder if show engint assets option off
						if (show_engine_assets ||
							(filename.find("asset/engine") == std::string::npos &&
							filename.find("asset\\engine") == std::string::npos))
						{
							folder_node.child_folders.push_back(child_offset++);
							folder_queue.push(filename);
						}
					}
				}
				folder_node.is_leaf = folder_node.child_folders.empty();
			}
		}

		// update folder opened status
		for (const FolderNode& folder_node : m_folder_nodes)
		{
			if (m_folder_opened_map.find(folder_node.name) == m_folder_opened_map.end())
			{
				m_folder_opened_map[folder_node.name] = false;
			}
		}

		// update selected folder's files
		openFolder("");
	}

	void IFolderTreeUI::constructFolderTree()
	{
		if (!m_folder_nodes.empty())
		{
			constructFolderTree(m_folder_nodes, 0);
		}
	}

	void IFolderTreeUI::constructFolderTree(const std::vector<FolderNode>& folder_nodes, uint32_t index)
	{
		const FolderNode& folder_node = folder_nodes[index];

		ImGuiTreeNodeFlags tree_node_flags = 0;
		tree_node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (folder_node.is_root)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}
		else if (folder_node.is_leaf)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (folder_node.dir == m_selected_folder)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool is_treenode_opened = ImGui::TreeNodeEx((void*)(intptr_t)index, tree_node_flags, "%s %s", m_folder_opened_map[folder_node.name] ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, folder_node.name.c_str());
		m_folder_opened_map[folder_node.name] = is_treenode_opened && !folder_node.is_leaf;
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			openFolder(folder_node.dir);
		}

		if (ImGui::IsItemHovered())
		{
			is_folder_tree_hovered |= true;
		}

		if (is_treenode_opened)
		{
			if (!folder_node.child_folders.empty())
			{
				const float k_unindent_w = 8;
				ImGui::Unindent(k_unindent_w);

				for (uint32_t child_folder : folder_node.child_folders)
				{
					constructFolderTree(folder_nodes, child_folder);
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

	bool IFolderTreeUI::createFolder()
	{
		LOG_INFO("create folder");
		return true;
	}

	bool IFolderTreeUI::deleteFolder()
	{
		LOG_INFO("delete folder");
		return true;
	}
}