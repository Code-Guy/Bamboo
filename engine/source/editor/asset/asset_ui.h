#pragma once

#include "editor/base/editor_ui.h"
#include "file_watcher.h"

namespace Bamboo
{
	class AssetUI : public EditorUI
	{
	public:
		AssetUI() = default;

		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:
		void constructFolderTree(const std::vector<FolderNode>& folder_nodes, uint32_t index = 0);

		// file watcher
		std::unique_ptr<FileWatcher> m_file_watcher;

		// icon images
		std::shared_ptr<ImGuiImage> m_closed_folder_image;
		std::shared_ptr<ImGuiImage> m_opened_folder_image;
		std::shared_ptr<ImGuiImage> m_asset_image;
	};
}