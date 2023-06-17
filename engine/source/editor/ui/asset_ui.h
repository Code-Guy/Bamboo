#pragma once

#include "editor_ui.h"
#include "runtime/platform/file/file_watcher.h"

namespace Bamboo
{
	class AssetUI : public EditorUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:
		std::shared_ptr<TSQueue<std::pair<std::string, EFileStatus>>> m_file_status_queue;
		std::unique_ptr<FileWatcher> m_file_watcher;
	};
}