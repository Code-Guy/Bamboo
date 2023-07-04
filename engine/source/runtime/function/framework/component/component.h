#pragma once

#include <memory>
#include <string>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>

#include "runtime/resource/serialization/serialization.h"

namespace Bamboo
{
	class Entity;
	class Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;

		void attach(std::shared_ptr<Entity>& parent);
		void dettach();
		std::shared_ptr<Entity>& getParent() { return m_parent; }

		virtual void tick(float delta_time) {}
		virtual void inflate() {}

	protected:
		std::shared_ptr<Entity> m_parent;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{

		}
	};
}