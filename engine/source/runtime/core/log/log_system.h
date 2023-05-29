#pragma once

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <memory>

namespace Bamboo
{
    class LogSystem
    {
        public:
            enum class LogLevel
            {
                Debug, Info, Warning, Error, Fatal
            };

            void init();
            void destroy();
        
            template<typename... TARGS>
            void log(LogLevel level, TARGS&&... args)
            {
                switch (level)
                {
				case LogLevel::Debug:
					m_logger->debug(std::forward<TARGS>(args)...);
					break;
				case LogLevel::Info:
					m_logger->info(std::forward<TARGS>(args)...);
					break;
				case LogLevel::Warning:
					m_logger->warn(std::forward<TARGS>(args)...);
					break;
				case LogLevel::Error:
					m_logger->error(std::forward<TARGS>(args)...);
					break;
				case LogLevel::Fatal:
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
            std::shared_ptr<spdlog::logger> m_logger;
    };
}
