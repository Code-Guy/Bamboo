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

		void setName(const std::string& name) { m_name = name; }
		const std::string& getName() { return m_name; }
		std::shared_ptr<Entity>& getParent() { return m_parent; }

	protected:
		std::string m_name;
		std::shared_ptr<Entity> m_parent;
	};
}