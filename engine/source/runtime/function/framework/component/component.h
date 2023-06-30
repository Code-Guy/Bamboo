#pragma once

#include <memory>
#include <string>

namespace Bamboo
{
	class Entity;
	class Component
	{
	public:
		Component(std::shared_ptr<Entity> parent) : m_parent(parent) {}
		virtual ~Component() = default;

		virtual void tick(float delta_time) {}

		std::shared_ptr<Entity>& getParent() { return m_parent; }

	protected:
		std::shared_ptr<Entity> m_parent;
	};
}