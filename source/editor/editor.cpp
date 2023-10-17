#include "editor.h"
#include "editor/global/editor_context.h"
#include "editor/menu/menu_ui.h"
#include "editor/tool/tool_ui.h"
#include "editor/world/world_ui.h"
#include "editor/property/property_ui.h"
#include "editor/simulation/simulation_ui.h"
#include "editor/asset/asset_ui.h"
#include "editor/log/log_ui.h"

#include "engine/engine.h"
#include "engine/core/base/macro.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include "engine/core/event/event_system.h"
#include "engine/resource/asset/asset_manager.h"

namespace Bamboo
{
    void Editor::init()
    {
        // init engine
        m_engine = new Bamboo::Engine;
        m_engine->init();

        // create editor ui
        std::shared_ptr<EditorUI> menu_ui = std::make_shared<MenuUI>();
        std::shared_ptr<EditorUI> tool_ui = std::make_shared<ToolUI>();
        std::shared_ptr<EditorUI> world_ui = std::make_shared<WorldUI>();
        std::shared_ptr<EditorUI> property_ui = std::make_shared<PropertyUI>();
        std::shared_ptr<EditorUI> asset_ui = std::make_shared<AssetUI>();
        std::shared_ptr<EditorUI> log_ui = std::make_shared<LogUI>();
        m_simulation_ui = std::make_shared<SimulationUI>();
        m_editor_uis = { menu_ui, tool_ui, world_ui, property_ui, asset_ui, m_simulation_ui, log_ui };

        // init all editor uis
		for (auto& editor_ui : m_editor_uis)
		{
			editor_ui->init();
		}

        // set construct ui function to UIPass through RenderSystem
        g_engine.eventSystem()->addListener(EEventType::RenderConstructUI, [this](const EventPointer& event) { constructUI(); });
    }

    void Editor::destroy()
    {
		// wait all gpu operations done
		VulkanRHI::get().waitDeviceIdle();

		// destroy all editor uis
		for (auto& editor_ui : m_editor_uis)
		{
			editor_ui->destroy();
		}

        // destroy engine
        m_engine->destroy();
        delete m_engine;
    }

    void Editor::run()
    {
        while (true)
        {
            // get delta time
            float delta_time = m_engine->calcDeltaTime();

            // tick editor
            // TODO

            // tick engine
            if (!m_engine->tick(delta_time))
            {
                return;
            }
        }
    }

	void Editor::constructUI()
	{
        if (g_editor.isSimulationPanelFullscreen())
        {
            m_simulation_ui->construct();
        }
        else
        {
			for (auto& editor_ui : m_editor_uis)
			{
				editor_ui->construct();
			}
        }
	}

}