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
		m_current_world_url = g_runtime_context.configManager()->getDefaultWorldUrl();
		loadWorld(m_current_world_url);
	}

	void WorldManager::destroy()
	{
		m_current_world.reset();
	}

	void WorldManager::tick(float delta_time)
	{
		if (m_current_world)
		{
			m_current_world->tick(delta_time);
		}
	}

	bool WorldManager::loadWorld(const URL& url)
	{
		if (m_current_world)
		{
			m_current_world.reset();
		}

		//m_current_world = g_runtime_context.assetManager()->loadAsset<World>(url);
		m_current_world_url = url;

		std::shared_ptr<SkeletalMesh> skeletal_mesh = g_runtime_context.assetManager()->loadAsset<SkeletalMesh>("asset/temp/skm_Cesium_Man.skm");
		//std::shared_ptr<Texture2D> texture = g_runtime_context.assetManager()->loadAsset<Texture2D>("asset/temp/tex_cesium_man_0.tex");
		//std::shared_ptr<Animation> animation = g_runtime_context.assetManager()->loadAsset<Animation>("asset/temp/anim_cesium_man_0.anim");

		return true;
	}

}