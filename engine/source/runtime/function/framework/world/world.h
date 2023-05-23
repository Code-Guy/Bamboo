#pragma once

#include "runtime/function/framework/entity/entity.h"
#include <unordered_map>

namespace Bamboo
{
	class World
	{
	public:
		void load(const std::string& url);
		void unload();

		void tick(float delta_time);

		const auto& getEntities() const { return m_entites; }
		std::weak_ptr<Entity> getEntity(EntityID id);
		bool removeEntity(EntityID id);

	private:
		std::unordered_map<EntityID, std::shared_ptr<Entity>> m_entites;
	};
}