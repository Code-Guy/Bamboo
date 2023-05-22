#pragma once

#include <memory>
#include <vector>

namespace Bamboo
{
    class FileSystem;
    class ConfigManager;
    class LogSystem;
    class WindowSystem;
    class WorldManager;

    class RuntimeContext
    {
        public:
            void init();
            void destroy();

			const auto& logSystem() { return m_log_system; }
			const auto& fileSystem() { return m_file_system; }
            const auto& configManager() { return m_config_manager; }
            const auto& windowSystem() { return m_window_system; }
            const auto& worldManager() { return m_world_manager; }

        private:
			std::shared_ptr<LogSystem> m_log_system;
			std::shared_ptr<FileSystem> m_file_system;
            std::shared_ptr<ConfigManager> m_config_manager;
			std::shared_ptr<WindowSystem> m_window_system;
			std::shared_ptr<WorldManager> m_world_manager;
    };

    extern RuntimeContext g_runtime_context;
}
