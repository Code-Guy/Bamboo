#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <stdexcept>
#include <memory>

namespace Bamboo
{
	enum class ELogLevel
	{
		Debug, Info, Warning, Error, Fatal
	};

    class LogSystem
    {
        public:
            void init();
            void destroy();
			std::vector<std::string> getLastestLogs();

            template<typename... TARGS>
            void log(ELogLevel level, TARGS&&... args)
            {
                switch (level)
                {
				case ELogLevel::Debug:
					m_logger->debug(std::forward<TARGS>(args)...);
					break;
				case ELogLevel::Info:
					m_logger->info(std::forward<TARGS>(args)...);
					break;
				case ELogLevel::Warning:
					m_logger->warn(std::forward<TARGS>(args)...);
					break;
				case ELogLevel::Error:
					m_logger->error(std::forward<TARGS>(args)...);
					break;
				case ELogLevel::Fatal:
				{
					m_logger->critical(args...);

					// throw application runtime error
					std::string fatal_str = fmt::format(std::forward<TARGS>(args)...);
					throw std::runtime_error(fatal_str);
				}
					break;
				default:
					break;
                }
            }

        private:
			std::string getLogFilename();

			std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> m_ringbuffer_sink;
            std::shared_ptr<spdlog::logger> m_logger;
    };
}
