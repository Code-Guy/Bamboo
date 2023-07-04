#pragma once

#include "runtime/function/framework/component/component.h"

#include <vector>
#include <atomic>
#include <limits>

#include <cereal/types/vector.hpp>

namespace Bamboo
{
	class World;
	class Entity : public std::enable_shared_from_this<Entity>
	{
	public:
		~Entity();

		virtual void tick(float delta_time);
		void inflate();

		void attach(std::weak_ptr<Entity>& parent);
		void detach();

		uint32_t getID() { return m_id; }
		std::weak_ptr<World> getWorld() { return m_world; }
		const std::string& getName() const { return m_name; }
		std::weak_ptr<Entity>& getParent() { return m_parent; }
		const auto& getComponents() const { return m_components; }

		void addComponent(std::shared_ptr<Component> component);
		void removeComponent(std::shared_ptr<Component> component);

		template<typename TComponent>
		bool hasComponent() const
		{
			for (auto& component : m_components)
			{
				if (std::dynamic_pointer_cast<TComponent>(component))
				{
					return true;
				}
			}
			return false;
		}

		template<typename TComponent>
		std::shared_ptr<TComponent> getComponent()
		{
			for (auto& component : m_components)
			{
				std::shared_ptr<TComponent> find_component = std::dynamic_pointer_cast<TComponent>(component);
				if (find_component)
				{
					return find_component;
				}
			}

			return nullptr;
		}

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("id", m_id));
			ar(cereal::make_nvp("parent_id", m_pid));
			ar(cereal::make_nvp("child_ids", m_cids));
			ar(cereal::make_nvp("components", m_components));
		}

		friend World;
		Entity() = default;

		uint32_t m_id;
		uint32_t m_pid = UINT_MAX;
		std::vector<uint32_t> m_cids;

		std::string m_name;
		std::weak_ptr<World> m_world;
		std::weak_ptr<Entity> m_parent;
		std::vector<std::weak_ptr<Entity>> m_children;
		std::vector<std::shared_ptr<Component>> m_components;
	};
}