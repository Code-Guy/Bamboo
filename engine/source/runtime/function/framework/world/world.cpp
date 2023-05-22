#include "world.h"

namespace Bamboo
{
	void World::load(const std::string& url)
	{

	}

	void World::unload()
	{
		m_entites.clear();
	}

	void World::tick(float delta_time)
	{
		for (const auto& iter : m_entites)
		{
			iter.second->tick(delta_time);
		}
	}

	std::weak_ptr<Entity> World::getEntity(EntityID id)
	{
		auto iter = m_entites.find(id);
		if (iter != m_entites.end())
		{
			return iter->second;
		}

		return std::weak_ptr<Entity>();
	}

	bool World::removeEntity(EntityID id)
	{
		return m_entites.erase(id) > 0;
	}

}