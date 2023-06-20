#include "file_watcher.h"
#include "runtime/core/base/macro.h"
#include <queue>

namespace Bamboo
{

	void FileWatcher::init(const std::string& root_folder, std::chrono::duration<int, std::milli> poll_delay)
	{
		m_root_folder = root_folder;
		m_poll_delay = poll_delay;
	}

	void FileWatcher::start()
	{
		m_is_running = true;
		m_thread = std::thread([this] { run(); });
	}

	void FileWatcher::stop()
	{
		m_is_running = false;
		m_thread.join();
	}

	std::vector<FolderNode> FileWatcher::getFolderNodes()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_cached_folder_nodes;
	}

	void FileWatcher::run()
	{
		while (m_is_running)
		{
			// recursively traverse the root folder by a queue
			const auto& fs = g_runtime_context.fileSystem();
			m_folder_nodes.clear();
			std::queue<std::string> folder_queue;
			folder_queue.push(m_root_folder);
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
					folder_node.name = fs->basename(folders[i]);
					folder_node.is_root = folders[i] == m_root_folder;
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

			// copy to cached folder nodes
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_cached_folder_nodes = m_folder_nodes;
			}

			// sleep for some time
			std::this_thread::sleep_for(m_poll_delay);
		}
	}

}