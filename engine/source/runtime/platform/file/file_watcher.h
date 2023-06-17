#pragma once

#include "runtime/platform/container/ts_queue.h"

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>

namespace Bamboo
{
	enum class EFileStatus
	{
		Created, Modified, Removed
	};

	class FileWatcher
	{
	public:
		void init(std::shared_ptr<TSQueue<std::pair<std::string, EFileStatus>>> file_status_queue, const std::string& root_path,
			std::chrono::duration<int, std::milli> poll_delay = std::chrono::milliseconds(1000));
		void start();
		void stop();

	private:
		void run();

		bool m_is_running;
		std::thread m_thread;

		std::string m_root_path;
		std::chrono::duration<int, std::milli> m_poll_delay;
		std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;
		std::shared_ptr<TSQueue<std::pair<std::string, EFileStatus>>> m_file_status_queue;
	};
}
