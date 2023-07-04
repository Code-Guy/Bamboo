#include "entity.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/world/world.h"

namespace Bamboo
{

	Entity::~Entity()
	{
		for (auto& component : m_components)
		{
			component.reset();
		}
		m_components.clear();
	}

	void Entity::tick(float delta_time)
	{
		for (auto& component : m_components)
		{
			component->tick(delta_time);
		}
	}

	void Entity::inflate()
	{
		for (auto& component : m_components)
		{
			component->attach(weak_from_this());
			component->inflate();
		}

		m_parent = m_world.lock()->getEntity(m_pid);
		for (uint32_t cid : m_cids)
		{
			m_children.push_back(m_world.lock()->getEntity(cid));
		}
	}

	void Entity::attach(std::weak_ptr<Entity>& parent)
	{
		m_parent = parent;
		m_parent.lock()->m_children.push_back(weak_from_this());
	}

	void Entity::detach()
	{
		auto& children = m_parent.lock()->m_children;
		children.erase(std::remove_if(children.begin(), children.end(), [this](const auto& child) {
			return child.lock()->m_id == m_id;
			}), children.end());
		m_parent.reset();
	}

	void Entity::addComponent(std::shared_ptr<Component> component)
	{
		component->attach(weak_from_this());
		m_components.push_back(component);
	}

	void Entity::removeComponent(std::shared_ptr<Component> component)
	{
		component->dettach();
		m_components.erase(std::remove(m_components.begin(), m_components.end(), component), m_components.end());
	}

}