#include "file_watcher.h"

namespace Bamboo
{

	void FileWatcher::init(std::shared_ptr<TSQueue<std::pair<std::string, EFileStatus>>> file_status_queue, const std::string& root_path,
		std::chrono::duration<int, std::milli> poll_delay /*= std::chrono::milliseconds(1000)*/)
	{
		m_file_status_queue = file_status_queue;
		m_root_path = root_path;
		m_poll_delay = poll_delay;

		for (const auto& file : std::filesystem::recursive_directory_iterator(m_root_path))
		{
			m_paths[file.path().string()] = std::filesystem::last_write_time(file);
		}
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

	void FileWatcher::run()
	{
		if (!m_file_status_queue)
		{
			return;
		}

		while (m_is_running)
		{
			std::this_thread::sleep_for(m_poll_delay);

			// check if a file was removed
			for (auto iter = m_paths.begin(); iter != m_paths.end(); )
			{
				if (!std::filesystem::exists(iter->first))
				{
					m_file_status_queue->push(std::make_pair(iter->first, EFileStatus::Removed));
					iter = m_paths.erase(iter);
				}
				else
				{
					iter++;
				}
			}

			// check if a file was created or modified
			for (auto& file : std::filesystem::recursive_directory_iterator(m_root_path))
			{
				auto current_file_last_write_time = std::filesystem::last_write_time(file);

				if (m_paths.find(file.path().string()) == m_paths.end())
				{
					// file creation
					m_paths[file.path().string()] = current_file_last_write_time;
					m_file_status_queue->push(std::make_pair(file.path().string(), EFileStatus::Created));
				}
				else
				{
					// file modification
					if (m_paths[file.path().string()] != current_file_last_write_time)
					{
						m_paths[file.path().string()] = current_file_last_write_time;
						m_file_status_queue->push(std::make_pair(file.path().string(), EFileStatus::Modified));
					}
				}
			}
		}
	}
}