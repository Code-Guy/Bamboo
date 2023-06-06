#include "editor.h"
#include "runtime/engine.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include "runtime/resource/asset/asset_manager.h"

namespace Bamboo
{
    void Editor::init(Engine *engine)
    {
        m_engine = engine;

        g_runtime_context.windowSystem()->registerOnDropFunc(std::bind(&Editor::onDrop, this, 
            std::placeholders::_1, std::placeholders::_2));
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

	void Editor::onDrop(int n, const char** filenames)
	{
        for (int i = 0; i < n; ++i)
        {
            g_runtime_context.assetManager()->importAsset(filenames[i], "asset/temp");
        }
	}

}