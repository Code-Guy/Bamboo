#include "world_manager.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/config/config_manager.h"
#include "runtime/resource/asset/asset_manager.h"

namespace Bamboo
{

	void WorldManager::init()
	{
		m_active_world_url = g_runtime_context.configManager()->getDefaultWorldUrl();
		loadWorld(m_active_world_url);
	}

	void WorldManager::destroy()
	{
		m_active_world.reset();
	}

	void WorldManager::tick(float delta_time)
	{
		if (m_active_world)
		{
			m_active_world->tick(delta_time);
		}
	}

	bool WorldManager::loadWorld(const URL& url)
	{
		if (m_active_world)
		{
			m_active_world.reset();
		}

		m_active_world = g_runtime_context.assetManager()->loadAsset<World>(url);
		m_active_world_url = url;

		return true;
	}

}