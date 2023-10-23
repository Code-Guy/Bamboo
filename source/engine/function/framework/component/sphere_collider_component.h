#pragma once

#include "collider_component.h"

namespace Bamboo
{
	class SphereColliderComponent : public ColliderComponent
	{
	public:
		SphereColliderComponent();

		float m_radius;

	private:
		REGISTER_REFLECTION(ColliderComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("collider_component", cereal::base_class<ColliderComponent>(this)));
			ar(cereal::make_nvp("radius", m_radius));
		}
	};
}