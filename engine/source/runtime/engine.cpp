#include "engine.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/function/render/window_system.h"

namespace Bamboo
{
    void Engine::init()
    {
		m_fps = 0;
		m_frame_count = 0;
		m_average_duration = 0.0f;
		m_last_tick_time_point = std::chrono::steady_clock::now();

        g_runtime_context.init();
        LOG_INFO("start engine");
    }

    void Engine::destroy()
    {
        LOG_INFO("stop engine");
        g_runtime_context.destroy();
    }

    bool Engine::tick(float delta_time)
    {
        logicTick(delta_time);
        calcFPS(delta_time);

        renderTick(delta_time);

        g_runtime_context.windowSystem()->pollEvents();
        g_runtime_context.windowSystem()->setTitle(std::string("BambooEngine - " + std::to_string(getFPS()) + " FPS"));

        return !g_runtime_context.windowSystem()->shouldClose();
    }

    float Engine::calcDeltaTime()
    {
        float delta_time;
        std::chrono::steady_clock::time_point tick_time_point = std::chrono::steady_clock::now();
        std::chrono::duration<float> time_span = std::chrono::duration_cast<std::chrono::duration<float>>(tick_time_point - m_last_tick_time_point);
        delta_time = time_span.count();
        m_last_tick_time_point = tick_time_point;

        return delta_time;
    }

    void Engine::logicTick(float delta_time)
    {
    }

    void Engine::renderTick(float delta_time)
    {
    }

    void Engine::calcFPS(float delta_time)
    {
        const float k_fps_alpha = 0.01f;
        if (m_frame_count++ == 0)
        {
            m_average_duration = delta_time;
        }
        else
        {
            m_average_duration = m_average_duration * (1 - k_fps_alpha) + delta_time * k_fps_alpha;
        }

        m_fps = static_cast<int>(1.0f / m_average_duration);
    }
}