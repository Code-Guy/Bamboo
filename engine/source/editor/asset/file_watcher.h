#pragma once

#include <string>
#include <thread>
#include <memory>
#include <vector>
#include <chrono>
#include <mutex>

namespace Bamboo
{
	struct FolderNode
	{
		std::string name;
		std::vector<std::string> child_files;
		std::vector<uint32_t> child_folders;
		bool is_root;
		bool is_leaf;
	};

	class FileWatcher
	{
	public:
		void init(const std::string& root_folder, std::chrono::duration<int, std::milli> poll_delay = std::chrono::milliseconds(1000));
		void start();
		void stop();

		std::vector<FolderNode> getFolderNodes();

	private:
		void run();

		bool m_is_running;
		std::thread m_thread;

		std::string m_root_folder;
		std::chrono::duration<int, std::milli> m_poll_delay;
		std::vector<FolderNode> m_folder_nodes;
		std::vector<FolderNode> m_cached_folder_nodes;
		std::mutex m_mutex;
	};
}
