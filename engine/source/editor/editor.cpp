#include "editor.h"
#include "runtime/engine.h"

namespace Bamboo
{
    void Editor::init(Engine *engine)
    {
        m_engine = engine;
    }

    void Editor::destroy()
    {
    }

    void Editor::run()
    {
        while (true)
        {
            float delta_time = m_engine->calcDeltaTime();
            if (!m_engine->tick(delta_time))
            {
                return;
            }
        }
    }
}