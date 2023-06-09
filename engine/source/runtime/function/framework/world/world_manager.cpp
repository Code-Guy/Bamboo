#include "world_manager.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/config/config_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/resource/asset/skeletal_mesh.h"
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/animation.h"

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

		//m_active_world = g_runtime_context.assetManager()->loadAsset<World>(url);
		m_active_world_url = url;

		std::shared_ptr<SkeletalMesh> skeletal_mesh = g_runtime_context.assetManager()->loadAsset<SkeletalMesh>("asset/temp/skm_Cesium_Man.skm");
		//std::shared_ptr<Texture2D> texture = g_runtime_context.assetManager()->loadAsset<Texture2D>("asset/temp/tex_cesium_man_0.tex");
		//std::shared_ptr<Animation> animation = g_runtime_context.assetManager()->loadAsset<Animation>("asset/temp/anim_cesium_man_0.anim");

		return true;
	}

}