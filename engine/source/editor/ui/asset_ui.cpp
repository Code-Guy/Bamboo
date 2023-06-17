#include "asset_ui.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{

	void AssetUI::init()
	{
		m_title = "Asset";

		m_file_status_queue = std::make_shared<TSQueue<std::pair<std::string, EFileStatus>>>();

		m_file_watcher = std::make_unique<class FileWatcher>();
		m_file_watcher->init(m_file_status_queue, g_runtime_context.fileSystem()->asset_dir());
		m_file_watcher->start();
	}

	void AssetUI::construct()
	{
		EditorUI::construct();

		// check if any file status changed(created/modified/removed)
		while (!m_file_status_queue->empty())
		{
			std::pair<std::string, EFileStatus> file_status_pair = m_file_status_queue->pop();
			const std::string& filename = file_status_pair.first;
			EFileStatus file_status = file_status_pair.second;
			LOG_INFO("file {} {}", filename, file_status == EFileStatus::Created ? "created" : 
				(file_status == EFileStatus::Modified ? "modified" : "removed"));
		}

		ImGui::Begin(m_title.c_str());

		for (int i = 0; i < 10; ++i)
		{
			ImGui::Text((m_title + std::to_string(i)).c_str());
		}

		ImGui::End();
	}

	void AssetUI::destroy()
	{
		m_file_watcher->stop();
	}

}