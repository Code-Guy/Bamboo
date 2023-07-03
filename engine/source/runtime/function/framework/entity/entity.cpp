#include "entity.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{
	Entity::Entity()
	{
		m_id = EntityIDAllocator::alloc();
	}

	Entity::~Entity()
	{

	}

	void Entity::tick(float delta_time)
	{
		for (auto& component : m_components)
		{
			component->tick(delta_time);
		}
	}

	std::atomic<EntityID> EntityIDAllocator::m_next_id = 0;
	constexpr EntityID k_invalid_entity_id = std::numeric_limits<std::size_t>::max();

	EntityID EntityIDAllocator::alloc()
	{
		std::atomic<EntityID> new_entity_id = m_next_id.load();
		m_next_id++;
		if (m_next_id >= k_invalid_entity_id)
		{
			LOG_FATAL("entity id overflow");
		}

		return new_entity_id;
	}

}