#pragma once

#include "collider_component.h"

namespace Bamboo
{
	class SphereColliderComponent : public ColliderComponent
	{
	public:
		SphereColliderComponent() { m_type = EColliderType::Sphere; }

		float m_radius = 1.0f;

	private:
		REGISTER_REFLECTION(ColliderComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<ColliderComponent>(this)));
			ar(cereal::make_nvp("radius", m_radius));
		}
	};
}