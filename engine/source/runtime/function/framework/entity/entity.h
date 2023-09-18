#pragma once

#include "runtime/function/framework/component/component.h"

#include <vector>
#include <atomic>
#include <limits>

#include <cereal/types/vector.hpp>

namespace Bamboo
{
	class World;
	class Entity : public std::enable_shared_from_this<Entity>, public ITickable
	{
	public:
		~Entity();

		virtual void tick(float delta_time) override;
		void inflate();

		bool isRoot() { return m_parent.expired(); }
		bool isLeaf() { return m_children.empty(); }

		void attach(std::weak_ptr<Entity>& parent);
		void detach();

		uint32_t getID() { return m_id; }
		const std::weak_ptr<World>& getWorld() { return m_world; }
		const std::string& getName() const { return m_name; }
		const std::weak_ptr<Entity>& getParent() { return m_parent; }
		const std::vector<std::weak_ptr<Entity>>& getChildren() { return m_children; }
		const auto& getComponents() const { return m_components; }

		void addComponent(std::shared_ptr<Component> component);
		void removeComponent(std::shared_ptr<Component> component);

		bool hasComponent(const std::string& type_name) const
		{
			for (auto& component : m_components)
			{
				if (component->getTypeName() == type_name)
				{
					return true;
				}
			}
			return false;
		}

		template<typename TComponent>
		std::shared_ptr<TComponent> getComponent(const std::string& type_name)
		{
			for (const auto& component : m_components)
			{
				if (component->getTypeName() == type_name)
				{
					return std::static_pointer_cast<TComponent>(component);
				}
			}

			return nullptr;
		}

#define getComponent(TComponent) getComponent<TComponent>(#TComponent)
#define hasComponent(TComponent) hasComponent(#TComponent)

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("tickable", cereal::base_class<ITickable>(this)));

			ar(cereal::make_nvp("name", m_name));
			ar(cereal::make_nvp("id", m_id));
			ar(cereal::make_nvp("parent_id", m_pid));
			ar(cereal::make_nvp("child_ids", m_cids));
			ar(cereal::make_nvp("components", m_components));
		}

		friend World;
		Entity() = default;

		void updateTransforms();

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