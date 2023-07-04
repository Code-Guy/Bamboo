#include "world.h"
#include "runtime/core/base/macro.h"
#include <fstream>

namespace Bamboo
{
	void World::tick(float delta_time)
	{
		for (const auto& iter : m_entities)
		{
			iter.second->tick(delta_time);
		}
	}

	void World::inflate()
	{
		for (const auto& iter : m_entities)
		{
			iter.second->inflate();
		}
	}

	std::shared_ptr<Entity> World::getEntity(uint32_t id)
	{
		auto iter = m_entities.find(id);
		if (iter != m_entities.end())
		{
			return iter->second;
		}

		return nullptr;
	}

	std::shared_ptr<Entity> World::createEntity(const std::string& name)
	{
		std::shared_ptr<Entity> entity = std::shared_ptr<Entity>(new Entity);
		entity->m_id = m_entity_counter++;
		entity->m_name = name;
		entity->m_world = this;

		m_entities[entity->m_id] = entity;
		return entity;
	}

	bool World::removeEntity(uint32_t id)
	{
		return m_entities.erase(id) > 0;
	}

}