#pragma once

#include <memory>
#include <vector>
#include <array>

namespace Bamboo
{
    class EditorContext
    {
        public:
            void init();
            void destroy();

            void toggleFullscreen() { m_fullscreen = !m_fullscreen; }
            bool isFullscreen() { return m_fullscreen; }

			void toggleSimulate() { m_simulate = !m_simulate; }
			bool isSimulate() { return m_simulate; }

        private:
            bool m_fullscreen = false;
            bool m_simulate = false;
    };

    extern EditorContext g_editor;
}
