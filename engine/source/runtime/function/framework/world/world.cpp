#include "world.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/component/camera_component.h"
#include <fstream>

CEREAL_REGISTER_TYPE(Bamboo::World)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::World)

namespace Bamboo
{

	World::~World()
	{
		m_camera_entity.reset();
		for (auto iter : m_entities)
		{
			iter.second.reset();
		}
		m_entities.clear();
	}

	void World::tick(float delta_time)
	{
		for (const auto& iter : m_entities)
		{
			// update entity's own and children transforms
			iter.second->updateTransforms();
			iter.second->tickable(delta_time);
		}
	}

	void World::inflate()
	{
		for (const auto& iter : m_entities)
		{
			const auto& entity = iter.second;
			entity->m_world = weak_from_this();
			entity->inflate();

			// get camera entity
			if (entity->hasComponent(CameraComponent))
			{
				m_camera_entity = entity;
			}

			// update next entity id
			m_next_entity_id = std::max(m_next_entity_id, entity->getID() + 1);
		}
	}

	std::weak_ptr<Entity> World::getEntity(uint32_t id)
	{
		const auto& iter = m_entities.find(id);
		if (iter != m_entities.end())
		{
			return iter->second;
		}

		return {};
	}

	std::weak_ptr<Entity> World::getEntity(const std::string& name)
	{
		for (const auto& entity : m_entities)
		{
			if (name == entity.second->getName())
			{
				return entity.second;
			}
		}
		return {};
	}

	const std::shared_ptr<Entity>& World::createEntity(const std::string& name)
	{
		std::shared_ptr<Entity> entity = std::shared_ptr<Entity>(new Entity);
		entity->m_id = m_next_entity_id++;
		entity->m_name = name;
		entity->m_world = weak_from_this();

		m_entities[entity->m_id] = entity;
		return m_entities[entity->m_id];
	}

	bool World::removeEntity(uint32_t id)
	{
		return m_entities.erase(id) > 0;
	}

}