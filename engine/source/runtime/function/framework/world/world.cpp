#include "world.h"
#include "runtime/core/base/macro.h"
#include <fstream>

namespace Bamboo
{
	void World::tick(float delta_time)
	{
		for (const auto& iter : m_entites)
		{
			iter.second->tick(delta_time);
		}
	}

	std::shared_ptr<Entity> World::getEntity(EntityID id)
	{
		auto iter = m_entites.find(id);
		if (iter != m_entites.end())
		{
			return iter->second;
		}

		return nullptr;
	}

	bool World::removeEntity(EntityID id)
	{
		return m_entites.erase(id) > 0;
	}

}