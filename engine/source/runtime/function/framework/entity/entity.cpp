#include "entity.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{
	std::atomic<EntityID> Entity::m_next_id = 0;

	Entity::Entity()
	{
		m_id = allocID();
	}

	Entity::~Entity()
	{

	}

	EntityID Entity::allocID()
	{
		std::atomic<EntityID> new_entity_id = m_next_id.load();
		m_next_id++;
		if (m_next_id >= k_invalid_entity_id)
		{
			LOG_FATAL("entity id overflow");
		}

		return new_entity_id;
	}

	void Entity::tick(float delta_time)
	{
		for (auto& component : m_components)
		{
			component->tick(delta_time);
		}
	}

}