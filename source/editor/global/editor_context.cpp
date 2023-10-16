#include "editor_context.h"
#include "engine/function/global/engine_context.h"
#include "engine/function/render/window_system.h"

namespace Bamboo
{
    EditorContext g_editor;

    void EditorContext::init()
    {
		
    }

    void EditorContext::destroy()
    {
		
	}

	void EditorContext::toggleFullscreen()
	{
		if (g_engine.isEditor())
		{
			m_simulation_panel_fullscreen = !m_simulation_panel_fullscreen;
		}
		else
		{
			g_engine.windowSystem()->toggleFullscreen();
		}
	}

	bool EditorContext::isSimulationPanelFullscreen()
	{
		return m_simulation_panel_fullscreen || g_engine.isApplication();
	}

}