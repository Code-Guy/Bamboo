#include "asset_ui.h"
#include "runtime/core/base/macro.h"
#include "runtime/platform/timer/timer.h"
#include <queue>

namespace Bamboo
{
	void AssetUI::init()
	{
		m_title = "Asset";

		// set poll folder timer
		const float k_poll_folder_time = 1.0f;
		m_poll_folder_timer_handle = g_runtime_context.timerManager()->addTimer(k_poll_folder_time, [this](){ pollFolders(); }, true, true);

		// load icon images
		const auto& fs = g_runtime_context.fileSystem();
		m_asset_images[EAssetType::Invalid] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/invalid.png"));
		m_asset_images[EAssetType::Texture2D] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/texture_2d.png"));
		m_asset_images[EAssetType::TextureCube] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/texture_cube.png"));
		m_asset_images[EAssetType::Material] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/material.png"));
		m_asset_images[EAssetType::Skeleton] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/skeleton.png"));
		m_asset_images[EAssetType::StaticMesh] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/static_mesh.png"));
		m_asset_images[EAssetType::SkeletalMesh] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/skeletal_mesh.png"));
		m_asset_images[EAssetType::Animation] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/animation.png"));
		m_asset_images[EAssetType::Font] = loadImGuiImage(fs->absolute("asset/engine/texture/asset/font.png"));
		m_empty_folder_image = loadImGuiImage(fs->absolute("asset/engine/texture/asset/empty_folder.png"));
		m_non_empty_folder_image = loadImGuiImage(fs->absolute("asset/engine/texture/asset/non_empty_folder.png"));
	}

	void AssetUI::construct()
	{
		EditorUI::construct();

		// render asset widget
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
		ImGui::Begin(combine(ICON_FA_FAN, m_title).c_str());

		const float k_folder_tree_width_scale = 0.2f;
		const uint32_t k_spacing = 4;
		ImVec2 content_size = ImGui::GetContentRegionAvail();
		content_size.x -= k_spacing;

		// folder tree
		ImGui::BeginChild("folder_tree", ImVec2(content_size.x * k_folder_tree_width_scale, content_size.y), true);
		ImGui::Spacing();
		if (!m_folder_nodes.empty())
		{
			constructFolderTree(m_folder_nodes);
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// folder files
		ImGui::BeginChild("folder_files", ImVec2(content_size.x * (1 - k_folder_tree_width_scale), content_size.y), true);

		ImGui::Spacing();
		ImGui::Indent(k_spacing);

		ImGui::BeginChild("asset_navigator", ImVec2(content_size.x * (1 - k_folder_tree_width_scale) - k_spacing * 3, 24), true);
		constructAssetNavigator();
		ImGui::EndChild();

		ImGui::BeginChild("folder_files");
		ImGui::Indent(k_spacing);
		constructFolderFiles();
		ImGui::EndChild();

		ImGui::EndChild();

		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
	}

	void AssetUI::destroy()
	{
		EditorUI::destroy();

		g_runtime_context.timerManager()->removeTimer(m_poll_folder_timer_handle);
	}

	void AssetUI::constructFolderTree(const std::vector<FolderNode>& folder_nodes, uint32_t index)
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
		if (folder_node.name == m_selected_folder)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool is_treenode_opened = ImGui::TreeNodeEx((void*)(intptr_t)index, tree_node_flags, "%s %s", m_folder_opened_map[folder_node.name] ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, folder_node.name.c_str());
		m_folder_opened_map[folder_node.name] = is_treenode_opened && !folder_node.is_leaf;
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			m_selected_folder = folder_node.dir;
			pollSelectedFolder();
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

	void AssetUI::constructAssetNavigator()
	{
		ImVec2 button_size(20, 20);
		ImGui::Button(ICON_FA_ARROW_LEFT, button_size);

		ImGui::SameLine();
		ImGui::Button(ICON_FA_ARROW_RIGHT, button_size);

		ImGui::SameLine();
		static char str1[128] = "";
		ImGui::PushItemWidth(200.0f);
		ImGui::InputTextWithHint("##search_asset", combine(ICON_FA_SEARCH, "Search...").c_str(), str1, IM_ARRAYSIZE(str1));
		ImGui::PopItemWidth();

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
		ImGui::Text(getFormattedFolderStr().c_str());
	}

	void AssetUI::constructFolderFiles()
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);

		ImVec2 icon_size(80, 80);
		ImGuiStyle& style = ImGui::GetStyle();
		float max_pos_x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15.0f, 24.0f));
		for (size_t i = 0; i < m_selected_files.size(); ++i)
		{
			constructAsset(m_selected_files[i], icon_size);

			float current_pos_x = ImGui::GetItemRectMax().x;
			float next_pos_x = current_pos_x + style.ItemSpacing.x + icon_size.x;
			if (i < m_selected_files.size() - 1 && next_pos_x < max_pos_x)
			{
				ImGui::SameLine();
			}
		}
		ImGui::PopStyleVar();
	}

	void AssetUI::constructAsset(const std::string& filename, const ImVec2& size)
	{
		ImTextureID tex_id = nullptr;
		std::string basename = g_runtime_context.fileSystem()->basename(filename);

		if (g_runtime_context.fileSystem()->is_file(filename))
		{
			EAssetType asset_type = g_runtime_context.assetManager()->getAssetType(filename);
			tex_id = m_asset_images[asset_type]->desc_set;
		}
		else if (g_runtime_context.fileSystem()->is_dir(filename))
		{
			bool is_empty = g_runtime_context.fileSystem()->is_empty_dir(filename);
			tex_id = is_empty ? m_empty_folder_image->desc_set : m_non_empty_folder_image->desc_set;
		}
		
		ImGui::BeginGroup();

		// draw hovered/selected background rect
		HoverState& hover_state = m_selected_file_hover_states[filename];
		bool is_hovered = hover_state.is_hovered;
		bool is_selected = m_selected_file == filename;
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
			ImGui::GetWindowDrawList()->AddRectFilled(hover_state.rect_min, hover_state.rect_max, IM_COL32(color.x, color.y, color.z, color.w), 3.0f, draw_flags);
		}
		
		// draw image
		ImGui::Image(tex_id, size);

		// draw asset name text
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 20.0f);
		float text_width = ImGui::CalcTextSize(basename.c_str()).x;
		if (text_width > size.x)
		{
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + size.x);
			ImGui::Text(basename.c_str());
			ImGui::PopTextWrapPos();
		}
		else
		{
			ImGui::SetCursorPosX(ImGui::GetCursorPos().x + (size.x - text_width) * 0.5f);
			ImGui::Text(basename.c_str());
		}

		ImGui::EndGroup();

		// update asset hover and selection status
		hover_state.is_hovered = ImGui::IsItemHovered();
		const float k_spacing = 4;
		hover_state.rect_min = ImVec2(ImGui::GetItemRectMin().x - k_spacing, ImGui::GetItemRectMin().y - k_spacing);
		hover_state.rect_max = ImVec2(ImGui::GetItemRectMax().x + k_spacing, ImGui::GetItemRectMax().y + k_spacing);

		if (ImGui::IsItemClicked())
		{
			m_selected_file = filename;
		}
	}

	void AssetUI::pollFolders()
	{
		// recursively traverse the root folder by a queue
		const auto& fs = g_runtime_context.fileSystem();

		m_folder_nodes.clear();
		std::queue<std::string> folder_queue;
		folder_queue.push(fs->asset_dir());
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
				folder_node.is_root = folders[i] == fs->asset_dir();
				for (auto& file : std::filesystem::directory_iterator(folders[i]))
				{
					if (file.is_regular_file())
					{
						folder_node.child_files.push_back(fs->basename(file.path().string()));
					}
					else if (file.is_directory())
					{
						folder_node.child_folders.push_back(child_offset++);
						folder_queue.push(file.path().string());
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

		// update selected folder files
		pollSelectedFolder();
	}

	void AssetUI::pollSelectedFolder()
	{
		if (!m_selected_folder.empty())
		{
			m_selected_files = g_runtime_context.fileSystem()->traverse(m_selected_folder);
			for (const std::string& selected_file : m_selected_files)
			{
				if (m_selected_file_hover_states.find(selected_file) == m_selected_file_hover_states.end())
				{
					m_selected_file_hover_states[selected_file] = { false };
				}
			}
		}
	}

	std::string AssetUI::getFormattedFolderStr()
	{
		std::string rel_folder_str = g_runtime_context.fileSystem()->relative(m_selected_folder);
		replace_all(rel_folder_str, "/", std::string(" ") + ICON_FA_ANGLE_RIGHT + " ");
		replace_all(rel_folder_str, "\\", std::string(" ") + ICON_FA_ANGLE_RIGHT + " ");
		return rel_folder_str;
	}

}