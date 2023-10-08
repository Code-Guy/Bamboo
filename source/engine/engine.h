#pragma once 

#include <chrono>

namespace Bamboo
{
    class Engine
    {
        public:
            void init();
            void destroy();
            bool tick(float delta_time);

            float calcDeltaTime();
            int getFPS() { return m_fps; }

        private:
            void logicTick(float delta_time);
            void renderTick(float delta_time);

            void calcFPS(float delta_time);

            int m_fps;
            int m_frame_count;
            float m_average_duration;
            std::chrono::steady_clock::time_point m_last_tick_time_point;
    };
}