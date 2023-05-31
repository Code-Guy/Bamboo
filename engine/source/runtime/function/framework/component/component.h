#pragma once

#include <memory>
#include <string>

namespace Bamboo
{
	class Entity;
	class Component
	{
	public:
		virtual void init(std::shared_ptr<Entity> parent)
		{
			m_parent = parent;
		}

		virtual void tick(float delta_time) {}

		const std::string getName() { return name; }

	protected:
		std::shared_ptr<Entity> m_parent;
		std::string name;
	};
}