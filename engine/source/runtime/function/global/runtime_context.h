#pragma once

#include <memory>
#include <vector>

namespace Bamboo
{
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
            const auto& renderSystem() { return m_render_system; }

        private:
			std::shared_ptr<class LogSystem> m_log_system;
			std::shared_ptr<class FileSystem> m_file_system;
            std::shared_ptr<class ConfigManager> m_config_manager;
			std::shared_ptr<class WindowSystem> m_window_system;
			std::shared_ptr<class WorldManager> m_world_manager;
			std::shared_ptr<class RenderSystem> m_render_system;
    };

    extern RuntimeContext g_runtime_context;
}
