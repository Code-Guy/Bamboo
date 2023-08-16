#include "world_manager.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/config/config_manager.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/resource/asset/skeletal_mesh.h"
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/texture_cube.h"
#include "runtime/resource/asset/animation.h"
#include "runtime/function/framework/component/transform_component.h"
#include "runtime/function/framework/component/camera_component.h"
#include "runtime/function/framework/component/directional_light_component.h"
#include "runtime/function/framework/component/sky_light_component.h"
#include "runtime/function/framework/component/spot_light_component.h"

namespace Bamboo
{

	void WorldManager::init()
	{
		URL default_world_url = g_runtime_context.configManager()->getDefaultWorldUrl();

		//m_current_world = createWorld(default_world_url);
		loadWorld(default_world_url);
		//scriptWorld();
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

		std::shared_ptr<TransformComponent> transform_component = std::make_shared<TransformComponent>();
		transform_component->m_position = glm::vec3(12.0f, 4.0f, -2.0f);
		transform_component->m_rotation = glm::vec3(0.0f, -150.0f, -20.0f);

		std::shared_ptr<CameraComponent> camera_component = std::make_shared<CameraComponent>();
		camera_component->setTickEnabled(true);
		camera_component->m_fovy = 60.0f;
		camera_component->m_aspect_ratio = 1.778f;
		camera_component->m_near = 0.1f;
		camera_component->m_far = 1000.0f;
		camera_component->m_move_speed = 4.0f;
		camera_component->m_turn_speed = 0.1f;
		camera_component->m_zoom_speed = 2.0f;

		std::shared_ptr<Entity> camera_entity = world->createEntity("camera");
		camera_entity->addComponent(transform_component);
		camera_entity->addComponent(camera_component);
		camera_entity->setTickEnabled(true);

		world->m_camera_entity = camera_entity;
		world->inflate();

		return world;
	}

	std::shared_ptr<CameraComponent> WorldManager::getCameraComponent()
	{
		return m_current_world->getCameraEntity()->getComponent(CameraComponent);
	}

	void WorldManager::scriptWorld()
	{
		// add directional light
		std::shared_ptr<TransformComponent> transform_component = std::make_shared<TransformComponent>();
		transform_component->m_position = glm::vec3(0.0f, 5.0f, 0.0f);
		transform_component->m_rotation = glm::vec3(0.0f, -150.0f, -20.0f);

		std::shared_ptr<DirectionalLightComponent> directional_light_component = std::make_shared<DirectionalLightComponent>();
		directional_light_component->m_intensity = 0.5f;

		std::shared_ptr<Entity> directional_light_entity = m_current_world->createEntity("directional_light");
		directional_light_entity->addComponent(transform_component);
		directional_light_entity->addComponent(directional_light_component);

		// add sky light
		transform_component = std::make_shared<TransformComponent>();

		std::shared_ptr<SkyLightComponent> sky_light_component = std::make_shared<SkyLightComponent>();
		sky_light_component->m_intensity = 1.0f;

		auto sky_texture_cube = g_runtime_context.assetManager()->loadAsset<TextureCube>("asset/engine/texture/ibl/texc_skybox.texc");
		sky_light_component->setTextureCube(sky_texture_cube);

		std::shared_ptr<Entity> sky_light_entity = m_current_world->createEntity("sky_light");
		sky_light_entity->addComponent(transform_component);
		sky_light_entity->addComponent(sky_light_component);

		// add point light
		transform_component = std::make_shared<TransformComponent>();

		std::shared_ptr<PointLightComponent> point_light_component = std::make_shared<PointLightComponent>();
		point_light_component->m_intensity = 1.0f;

		std::shared_ptr<Entity> point_light_entity = m_current_world->createEntity("point_light");
		point_light_entity->addComponent(transform_component);
		point_light_entity->addComponent(point_light_component);

		// add spot light
		transform_component = std::make_shared<TransformComponent>();

		std::shared_ptr<SpotLightComponent> spot_light_component = std::make_shared<SpotLightComponent>();
		spot_light_component->m_intensity = 1.0f;

		std::shared_ptr<Entity> spot_light_entity = m_current_world->createEntity("spot_light");
		spot_light_entity->addComponent(transform_component);
		spot_light_entity->addComponent(spot_light_component);
	}

}