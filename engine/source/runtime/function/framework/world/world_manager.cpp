#include "world_manager.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/config/config_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/resource/asset/skeletal_mesh.h"
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/animation.h"
#include "runtime/function/framework/component/camera_component.h"

namespace Bamboo
{

	void WorldManager::init()
	{
		URL default_world_url = g_runtime_context.configManager()->getDefaultWorldUrl();

		m_current_world = createWorld(default_world_url);
		//loadWorld(default_world_url);
	}

	void WorldManager::destroy()
	{
		g_runtime_context.assetManager()->serializeAsset(m_current_world);
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

		m_current_world = g_runtime_context.assetManager()->loadAsset<World>(url);
		m_current_world_url = url;

		return true;
	}

	std::shared_ptr<World> WorldManager::createWorld(const URL& url)
	{
		std::shared_ptr<World> world = std::shared_ptr<World>(new World);
		world->setURL(url);

		std::shared_ptr<CameraComponent> camera_component = std::make_shared<CameraComponent>();
		camera_component->m_position = glm::vec3(8.5f, -1.9f, 3.9f);
		camera_component->m_yaw = -194.4f;
		camera_component->m_pitch = -18.7f;
		camera_component->m_fovy = 60.0f;
		camera_component->m_aspect_ratio = 1.778f;
		camera_component->m_near = 0.1f;
		camera_component->m_far = 1000.0f;
		camera_component->m_speed = 2.0f;
		camera_component->m_sensitivity = 0.1f;

		std::shared_ptr<Entity> camera_entity = world->createEntity("camera");
		camera_entity->addComponent(camera_component);

		world->m_camera_entity = camera_entity;
		world->inflate();

		return world;
	}

}