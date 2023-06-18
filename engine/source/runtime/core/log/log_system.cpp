#include "runtime/core/base/macro.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <ctime>

#define LOGGER_PATH "log"
#define MAX_LOG_FILE_NUM 100
#define MAX_ROTATE_FILE_NUM 5
#define MAX_ROTATE_FILE_SIZE 1048576 * 10

namespace Bamboo
{
    void LogSystem::init()
    {
        // console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%^%l%$] %v");

        // file sink
        std::string log_filename = getLogFilename();
		auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_filename, MAX_ROTATE_FILE_SIZE, MAX_ROTATE_FILE_NUM);
		file_sink->set_level(spdlog::level::trace);
		file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        // create multi-sink logger
        spdlog::init_thread_pool(8192, 1);

#ifdef DEBUG
        const spdlog::sinks_init_list sink_list = { console_sink, file_sink };
#else
        const spdlog::sinks_init_list sink_list = { file_sink };
#endif
        
        m_logger = std::make_shared<spdlog::async_logger>("multi_sink_logger",
            sink_list.begin(), sink_list.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
        m_logger->set_level(spdlog::level::trace);

        spdlog::register_logger(m_logger);
    }

    void LogSystem::destroy()
    {
        m_logger->flush();
        spdlog::drop_all();
    }

    std::string getCurrentDateTimeStr()
    {
		auto current_time = std::time(nullptr);
		auto local_time = *std::localtime(&current_time);
		std::ostringstream oss;
		oss << std::put_time(&local_time, "%Y-%m-%d-%H-%M-%S");
		return oss.str();
    }

    std::string LogSystem::getLogFilename()
	{
        std::string abs_logger_path = g_runtime_context.fileSystem()->absolute(LOGGER_PATH);
        if (!g_runtime_context.fileSystem()->exists(abs_logger_path))
        {
            g_runtime_context.fileSystem()->create_dir(abs_logger_path);
        }
        else
        {
            std::vector<std::string> log_filenames = g_runtime_context.fileSystem()->traverse(abs_logger_path, false, EFileOrderType::Time);
            if (log_filenames.size() >= MAX_LOG_FILE_NUM)
            {
                size_t removed_num = log_filenames.size() - MAX_LOG_FILE_NUM / 2;
                for (size_t i = 0; i < removed_num; ++i)
                {
                    g_runtime_context.fileSystem()->remove_file(log_filenames[i]);
                }
            }
        }

        std::string log_filename = g_runtime_context.fileSystem()->combine(abs_logger_path, "bamboo_" + getCurrentDateTimeStr() + ".log");
        return log_filename;
	}

}