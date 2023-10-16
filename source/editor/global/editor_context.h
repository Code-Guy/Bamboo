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

            void toggleFullscreen();
            bool isSimulationPanelFullscreen();

        private:
            bool m_simulation_panel_fullscreen = false;
    };

    extern EditorContext g_editor;
}
