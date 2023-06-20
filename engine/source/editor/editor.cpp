#include "editor.h"
#include "editor/menu/menu_ui.h"
#include "editor/world/world_ui.h"
#include "editor/property/property_ui.h"
#include "editor/simulation/simulation_ui.h"
#include "editor/asset/asset_ui.h"
#include "editor/log/log_ui.h"
#include "runtime/engine.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include "runtime/function/render/window_system.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/render/render_system.h"

namespace Bamboo
{
    void Editor::init(Engine *engine)
    {
        m_engine = engine;

        g_runtime_context.windowSystem()->registerOnDropFunc(std::bind(&Editor::onDrop, this, 
            std::placeholders::_1, std::placeholders::_2));

        // create editor ui
        std::shared_ptr<EditorUI> menu_ui = std::make_shared<MenuUI>();
        std::shared_ptr<EditorUI> world_ui = std::make_shared<WorldUI>();
        std::shared_ptr<EditorUI> property_ui = std::make_shared<PropertyUI>();
        std::shared_ptr<EditorUI> game_ui = std::make_shared<SimulationUI>();
        std::shared_ptr<EditorUI> asset_ui = std::make_shared<AssetUI>();
        std::shared_ptr<EditorUI> log_ui = std::make_shared<LogUI>();
        m_editor_uis = { menu_ui, world_ui, property_ui, game_ui, asset_ui, log_ui };

        // init all editor uis
		for (auto& editor_ui : m_editor_uis)
		{
			editor_ui->init();
		}

        // set construct ui function to UIPass through RenderSystem
        g_runtime_context.renderSystem()->setConstructUIFunc([this]() {
            for (auto& editor_ui : m_editor_uis)
            {
                editor_ui->construct();
            }
            });
    }

    void Editor::destroy()
    {
		// wait all gpu operations done
		vkDeviceWaitIdle(VulkanRHI::get().getDevice());

		// destroy all editor uis
		for (auto& editor_ui : m_editor_uis)
		{
			editor_ui->destroy();
		}
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

	void Editor::onDrop(int n, const char** filenames)
	{
        for (int i = 0; i < n; ++i)
        {
            g_runtime_context.assetManager()->importAsset(filenames[i], "asset/temp");
        }
	}

}