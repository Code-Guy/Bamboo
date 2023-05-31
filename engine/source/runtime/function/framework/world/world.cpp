#include "world.h"
#include "runtime/core/base/macro.h"
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Bamboo
{
	void World::load(const std::string& url)
	{
		std::ifstream f(REDIRECT(url));
		json data = json::parse(f);
		const auto& models = data["models"];
		for (const auto& model : models)
		{
			std::string url = model["url"].get<std::string>();
			bool is_combined = model["is_combined"].get<bool>();

		}
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

	std::shared_ptr<Entity> World::getEntity(EntityID id)
	{
		auto iter = m_entites.find(id);
		if (iter != m_entites.end())
		{
			return iter->second;
		}

		return std::shared_ptr<Entity>();
	}

	bool World::removeEntity(EntityID id)
	{
		return m_entites.erase(id) > 0;
	}

}