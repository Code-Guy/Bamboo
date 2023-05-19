#include "engine.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/runtime_context.h"

namespace Bamboo
{
    Engine::Engine()
    {
        m_fps = 0;
        m_frame_count = 0;
        m_average_duration = 0.0f;
        m_last_tick_time_point = std::chrono::steady_clock::now();
    }

    void Engine::init()
    {
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

        return true;
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