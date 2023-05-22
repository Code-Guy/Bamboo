#pragma once

#include <memory>
#include <string>

namespace Bamboo
{
	class Entity;
	class Component
	{
	public:
		virtual void init(std::weak_ptr<Entity> parent_entity)
		{
			m_parent_entity = parent_entity;
		}

		virtual void tick(float delta_time) {}

		const std::string getName() { return name; }

	protected:
		std::weak_ptr<Entity> m_parent_entity;
		std::string name;
	};
}