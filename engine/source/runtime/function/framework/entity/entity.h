#pragma once

#include "runtime/function/framework/component/component.h"

#include <vector>
#include <atomic>
#include <limits>

namespace Bamboo
{
	using EntityID = std::size_t;

	class Entity
	{
	public:
		Entity();
		virtual ~Entity();

		virtual void tick(float delta_time);

		EntityID getID() { return m_id; }
		void setName(const std::string& name) { m_name = name; }
		const std::string& getName() const { return m_name; }
		std::shared_ptr<Entity>& getParent() { return m_parent; }

		const auto& getComponents() const { return m_components; }

		template<typename TComponent>
		bool hasComponent() const
		{
			for (auto& component : m_components)
			{
				if (dynamic_cast<TComponent*>(component.get()))
				{
					return true;
				}
			}
			return false;
		}

		template<typename TComponent>
		TComponent* getComponent()
		{
			for (auto& component : m_components)
			{
				TComponent* find_component = dynamic_cast<TComponent*>(component.get());
				if (find_component)
				{
					return find_component;
				}
			}

			return nullptr;
		}

		template<typename TComponent>
		const TComponent* getComponent() const
		{
			for (const auto& component : m_components)
			{
				TComponent* find_component = dynamic_cast<TComponent*>(component.get());
				if (find_component)
				{
					return find_component;
				}
			}
			return nullptr;
		}

	private:
		EntityID m_id;
		std::string m_name;
		std::shared_ptr<Entity> m_parent;
		std::vector<std::shared_ptr<Component>> m_components;
	};

	class EntityIDAllocator
	{
	public:
		static EntityID alloc();

	private:
		static std::atomic<EntityID> m_next_id;
	};
}