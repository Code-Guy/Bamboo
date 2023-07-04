#include "entity.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/world/world.h"

namespace Bamboo
{
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
			component->attach(shared_from_this());
			component->inflate();
		}

		m_parent = m_world->getEntity(m_pid);
		for (uint32_t cid : m_cids)
		{
			m_children.push_back(m_world->getEntity(cid));
		}
	}

	void Entity::attach(std::shared_ptr<Entity>& parent)
	{
		m_parent = parent;
		m_parent->m_children.push_back(shared_from_this());
	}

	void Entity::detach()
	{
		m_parent->m_children.erase(std::remove(m_parent->m_children.begin(), 
			m_parent->m_children.end(), shared_from_this()), m_parent->m_children.end());
		m_parent = nullptr;
	}

	void Entity::addComponent(std::shared_ptr<Component> component)
	{
		component->attach(shared_from_this());
		m_components.push_back(component);
	}

	void Entity::removeComponent(std::shared_ptr<Component> component)
	{
		component->dettach();
		m_components.erase(std::remove(m_components.begin(), m_components.end(), component), m_components.end());
	}

}