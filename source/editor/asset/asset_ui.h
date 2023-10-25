#pragma once

#include "editor/base/editor_ui.h"
#include "editor/base/folder_tree_ui.h"
#include "engine/resource/asset/asset_manager.h"

namespace Bamboo
{
	class AssetUI : public EditorUI, public IFolderTreeUI
	{
	public:
		AssetUI() = default;

		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:
		void constructAssetNavigator();
		void constructFolderFiles();
		void constructAsset(const std::string& filename, const ImVec2& size);
		void constructImportPopups();
		void constructAssetFilePopups();
		void constructFolderOpPopups(const std::string& str_id, bool is_background_not_hoverd = false);

		virtual void openFolder(std::string folder) override;

		void onDropFiles(const std::shared_ptr<class Event>& event);
		void onAssetRightClick(const std::string& filename);

		void createCustomSeperatorText(const std::string& text);

		// icon images
		std::map<EAssetType, std::shared_ptr<ImGuiImage>> m_asset_images;
		std::shared_ptr<ImGuiImage> m_empty_folder_image;
		std::shared_ptr<ImGuiImage> m_non_empty_folder_image;

		// folder infos
		uint32_t m_poll_folder_timer_handle;
		std::string m_formatted_selected_folder;
		std::string m_selected_file;
		std::vector<std::string> m_selected_files;

		std::map<std::string, HoverState> m_selected_file_hover_states;

		// import files
		glm::vec4 m_folder_rect;
		std::vector<std::string> m_imported_files;

		bool is_asset_hovered = false;
	};
}