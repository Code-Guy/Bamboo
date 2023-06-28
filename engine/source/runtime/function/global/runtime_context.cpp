#include "runtime_context.h"
#include "runtime/platform/timer/timer.h"
#include "runtime/platform/file/file_system.h"
#include "runtime/resource/config/config_manager.h"
#include "runtime/core/log/log_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/render/render_system.h"
#include "runtime/resource/shader/shader_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{
    RuntimeContext g_runtime_context;

    void RuntimeContext::init()
    {
		m_timer_manager = std::make_shared<TimerManager>();
        m_timer_manager->init();

		m_file_system = std::make_shared<FileSystem>();
		m_file_system->init();

		m_log_system = std::make_shared<LogSystem>();
		m_log_system->init();

        m_config_manager = std::make_shared<ConfigManager>();
        m_config_manager->init();

        m_window_system = std::make_shared<WindowSystem>();
        m_window_system->init();

        VulkanRHI::get().init();

		m_shader_manager = std::make_shared<ShaderManager>();
        m_shader_manager->init();

		m_asset_manager = std::make_shared<AssetManager>();
		m_asset_manager->init();

        m_world_manager = std::make_shared<WorldManager>();
        m_world_manager->init();

        m_render_system = std::make_shared<RenderSystem>();
        m_render_system->init();
    }

    void RuntimeContext::destroy()
    {
		// wait all gpu operations done
        VulkanRHI::get().waitDeviceIdle();

        // destroy with reverse initialize order
        m_render_system->destroy();
        m_world_manager->destroy();
		m_asset_manager->destroy();
        m_shader_manager->destroy();
        VulkanRHI::get().destroy();
		m_window_system->destroy();
        m_config_manager->destroy();
        m_log_system->destroy();
		m_file_system->destroy();
        m_timer_manager->destroy();
	}
}