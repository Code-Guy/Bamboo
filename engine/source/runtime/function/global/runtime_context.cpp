#include "runtime_context.h"
#include "runtime/platform/file/file_system.h"
#include "runtime/resource/config/config_manager.h"
#include "runtime/core/log/log_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/render/render_system.h"

namespace Bamboo
{
    RuntimeContext g_runtime_context;

    void RuntimeContext::init()
    {
		m_log_system = std::make_shared<LogSystem>();
		m_log_system->init();

        m_file_system = std::make_shared<FileSystem>();
        m_file_system->init();

        m_config_manager = std::make_shared<ConfigManager>();
        m_config_manager->init(m_file_system->redirect("config/engine.yaml"));

        m_window_system = std::make_shared<WindowSystem>();
        WindowCreateInfo window_ci;
        window_ci.width = m_config_manager->getWindowWidth();
        window_ci.height = m_config_manager->getWindowHeight();
        window_ci.title = m_config_manager->getWindowTitle();
        m_window_system->init(window_ci);

        m_world_manager = std::make_shared<WorldManager>();
        m_world_manager->init();

        m_render_system = std::make_shared<RenderSystem>();
        m_render_system->init();
    }

    void RuntimeContext::destroy()
    {
        // destroy with reverse initialize order
        m_render_system->destroy();
        m_world_manager->destroy();
        m_window_system->destroy();
        m_config_manager->destroy();
        m_file_system->destroy();
        m_log_system->destroy();
    }
}