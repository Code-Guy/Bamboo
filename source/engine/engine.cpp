#include "engine.h"
#include "engine/core/base/macro.h"
#include "engine/platform/timer/timer.h"
#include "engine/core/event/event_system.h"
#include "engine/function/render/window_system.h"
#include "engine/function/render/render_system.h"
#include "engine/function/framework/world/world_manager.h"

namespace Bamboo
{
    void Engine::init()
    {
		m_fps = 0;
		m_frame_count = 0;
		m_average_duration = 0.0f;
		m_last_tick_time_point = std::chrono::steady_clock::now();

        g_engine.init();
        LOG_INFO("start engine");
    }

    void Engine::destroy()
    {
        LOG_INFO("stop engine");
        g_engine.destroy();
    }

    bool Engine::tick(float delta_time)
    {
        logicTick(delta_time);
        renderTick(delta_time);

		m_frame_count++;
		calcFPS(delta_time);

        g_engine.setDeltaTime(delta_time);
        g_engine.windowSystem()->pollEvents();
        g_engine.windowSystem()->setTitle(std::string(APP_NAME) + " - " + std::to_string(getFPS()) + " FPS");

        return !g_engine.windowSystem()->shouldClose();
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
        g_engine.eventSystem()->tick();
        g_engine.worldManager()->tick(delta_time);
		g_engine.timerManager()->tick(delta_time);
	}

    void Engine::renderTick(float delta_time)
    {
        g_engine.renderSystem()->tick(delta_time);
    }

    void Engine::calcFPS(float delta_time)
    {
        const float k_update_fps_time = 1.0f;
        static float update_fps_timer = 0.0f;
        if (update_fps_timer > k_update_fps_time)
        {
            m_fps = static_cast<int>(1.0f / delta_time);
            update_fps_timer -= k_update_fps_time;
        }
        else
        {
            update_fps_timer += delta_time;
        }
    }
}