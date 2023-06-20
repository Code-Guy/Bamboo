#include "asset_ui.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{
	void AssetUI::init()
	{
		m_title = "Asset";

		const auto& fs = g_runtime_context.fileSystem();
		m_file_watcher = std::make_unique<FileWatcher>();
		m_file_watcher->init(fs->asset_dir());
		m_file_watcher->start();

		// load icon images
		m_closed_folder_image = loadImGuiImage(fs->absolute("asset/engine/texture/closed_folder.png"));
	}

	void AssetUI::construct()
	{
		EditorUI::construct();

		// check if any file status changed
		std::vector<FolderNode> folder_nodes = m_file_watcher->getFolderNodes();

		// render asset widget
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
		ImGui::Begin(combine(ICON_FA_FAN, m_title).c_str());

		const float k_folder_tree_width_scale = 0.3f;
		const uint32_t k_spacing = 4;
		ImVec2 content_size = ImGui::GetContentRegionAvail();
		content_size.x -= k_spacing;

		// folder tree
		ImGui::BeginChild("folder_tree", ImVec2(content_size.x * k_folder_tree_width_scale, content_size.y), true);
		constructFolderTree(folder_nodes);
		ImGui::EndChild();

		ImGui::SameLine();

		// folder files
		ImGui::BeginChild("folder_files", ImVec2(content_size.x * (1 - k_folder_tree_width_scale), content_size.y), true);

		ImGui::EndChild();

		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
	}

	void AssetUI::destroy()
	{
		EditorUI::destroy();

		m_file_watcher->stop();
	}

	void AssetUI::constructFolderTree(const std::vector<FolderNode>& folder_nodes, uint32_t index)
	{
		ImGuiTreeNodeFlags tree_node_flags = 0;
		tree_node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

		const FolderNode& folder_node = folder_nodes[index];
		if (folder_node.is_root)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}
		else if (folder_node.is_leaf)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
		}

		const float k_unindent_w = 8;
		if (ImGui::TreeNodeEx((void*)(intptr_t)index, tree_node_flags, "%s %s", ICON_FA_FOLDER, folder_node.name.c_str()))
		{
			if (!folder_node.child_folders.empty())
			{
				ImGui::Unindent(k_unindent_w);

				for (uint32_t child_folder : folder_node.child_folders)
				{
					constructFolderTree(folder_nodes, child_folder);
				}

			}

			ImGui::TreePop();
		}
	}

}