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

		//m_current_world = createWorld(default_world_url);
		loadWorld(default_world_url);
	}

	void WorldManager::destroy()
	{
		//g_runtime_context.assetManager()->serializeAsset(m_current_world);
		m_current_world.reset();
	}

	void WorldManager::tick(float delta_time)
	{
		m_current_world->tick(delta_time);
	}

	bool WorldManager::loadWorld(const URL& url)
	{
		if (m_current_world)
		{
			m_current_world.reset();
		}

		m_current_world = g_runtime_context.assetManager()->loadAsset<World>(url);
		m_current_world_url = url;
		m_current_world_name = g_runtime_context.fileSystem()->basename(url);

		return true;
	}

	std::shared_ptr<World> WorldManager::createWorld(const URL& url)
	{
		std::shared_ptr<World> world = std::shared_ptr<World>(new World);
		world->setURL(url);

		std::shared_ptr<CameraComponent> camera_component = std::make_shared<CameraComponent>();
		camera_component->m_position = glm::vec3(3.0f, -3.0f, 3.0f);
		camera_component->m_yaw = -225.0f;
		camera_component->m_pitch = -35.2f;
		camera_component->m_fovy = 60.0f;
		camera_component->m_aspect_ratio = 1.778f;
		camera_component->m_near = 0.1f;
		camera_component->m_far = 1000.0f;
		camera_component->m_move_speed = 4.0f;
		camera_component->m_turn_speed = 0.1f;
		camera_component->m_zoom_speed = 2.0f;

		std::shared_ptr<Entity> camera_entity = world->createEntity("camera");
		camera_entity->addComponent(camera_component);

		world->m_camera_entity = camera_entity;
		world->inflate();

		return world;
	}

	std::shared_ptr<CameraComponent> WorldManager::getCameraComponent()
	{
		return m_current_world->getCameraEntity()->getComponent<CameraComponent>();
	}

}