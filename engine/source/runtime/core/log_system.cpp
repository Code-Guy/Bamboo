#include "log_system.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Bamboo
{
    void LogSystem::init()
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%^%l%$] %v");

        spdlog::init_thread_pool(8192, 1);
        const spdlog::sinks_init_list sink_list = {console_sink};
        m_logger = std::make_shared<spdlog::async_logger>("muggle_logger",
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
}