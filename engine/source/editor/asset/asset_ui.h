#pragma once

#include "editor/base/editor_ui.h"
#include "runtime/resource/asset/asset_manager.h"

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

	class AssetUI : public EditorUI
	{
	public:
		AssetUI() = default;

		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:
		void constructFolderTree(const std::vector<FolderNode>& folder_nodes, uint32_t index = 0);
		void constructAssetNavigator();
		void constructFolderFiles();
		void constructAsset(const std::string& filename, const ImVec2& size);

		void pollFolders();
		void pollSelectedFolder(const std::string& selected_folder = "");
		void pollImportFiles();

		void onDropFiles(int n, const char** filenames);

		// icon images
		std::map<EAssetType, std::shared_ptr<ImGuiImage>> m_asset_images;
		std::shared_ptr<ImGuiImage> m_empty_folder_image;
		std::shared_ptr<ImGuiImage> m_non_empty_folder_image;

		// folder infos
		uint32_t m_poll_folder_timer_handle;
		std::vector<FolderNode> m_folder_nodes;
		std::map<std::string, bool> m_folder_opened_map;

		std::string m_selected_folder;
		std::string m_formatted_selected_folder;
		std::string m_selected_file;
		std::vector<std::string> m_selected_files;

		struct HoverState
		{
			bool is_hovered;
			ImVec2 rect_min;
			ImVec2 rect_max;
		};

		std::map<std::string, HoverState> m_selected_file_hover_states;

		// import files
		std::vector<std::string> m_imported_files;
	};
}