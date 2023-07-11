#include "entity.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/framework/world/world.h"
#include "runtime/function/framework/component/transform_component.h"

#include <queue>

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
		// update own and children transforms
		updateTransforms();

		// tick components
		for (auto& component : m_components)
		{
			component->tickable(delta_time);
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

	void Entity::updateTransforms()
	{
		if (!isRoot())
		{
			return;
		}

		if (m_children.empty())
		{
			getComponent<TransformComponent>()->update();
			return;
		}

		std::queue<std::tuple<Entity*, bool, glm::mat4>> queue;
		queue.push(std::make_tuple(this, false, glm::mat4(1.0)));

		while (!queue.empty())
		{
			std::tuple<Entity*, bool, glm::mat4> tuple = queue.front();
			queue.pop();

			Entity* entity = std::get<0>(tuple);
			bool is_chain_dirty = std::get<1>(tuple);
			const glm::mat4& parent_global_matrix = std::get<2>(tuple);

			auto& transform_component = entity->getComponent<TransformComponent>();
			is_chain_dirty = transform_component->update(is_chain_dirty, parent_global_matrix);

			for (auto& child : entity->m_children)
			{
				queue.push(std::make_tuple(child.lock().get(), is_chain_dirty, transform_component->getGlobalMatrix()));
			}
		}
	}

}