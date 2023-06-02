#pragma once

#include "runtime/function/framework/component/component.h"

#include <vector>
#include <atomic>
#include <limits>

namespace Bamboo
{
	using EntityID = std::size_t;
	constexpr EntityID k_invalid_entity_id = std::numeric_limits<std::size_t>::max();

	class Entity
	{
	public:
		Entity(EntityID id) : m_id(id) {}
		virtual ~Entity();

		static EntityID allocID();

		virtual void tick(float delta_time);

		EntityID getID() { return m_id; }
		void setName(const std::string& name) { m_name = name; }
		const std::string& getName() const { return m_name; }
		std::shared_ptr<Entity>& getParent() { return m_parent; }

		bool hasComponent(const std::string& name) const;
		const auto& getComponents() const { return m_components; }

	private:
		template<typename TComponent>
		TComponent* tryGetComponent(const std::string& name)
		{
			for (auto& component : m_components)
			{
				if (component->getName() == name)
				{
					return static_cast<TComponent*>(component.get());
				}
			}

			return nullptr;
		}

		template<typename TComponent>
		const TComponent* tryGetComponentConst(const std::string& name) const
		{
			for (const auto& component : m_components)
			{
				if (component.getName() == name)
				{
					return static_cast<TComponent*>(component.get());
				}
			}
			return nullptr;
		}

#define tryGetComponent(COMPONENT_CLASS) tryGetComponent<COMPONENT_CLASS>(#COMPONENT_CLASS)
#define tryGetComponentConst(COMPONENT_CLASS) tryGetComponentConst<const COMPONENT_CLASS>(#COMPONENT_CLASS)

		EntityID m_id;
		std::string m_name;
		std::shared_ptr<Entity> m_parent;
		std::vector<std::shared_ptr<Component>> m_components;

		static std::atomic<EntityID> m_next_id;
	};
}