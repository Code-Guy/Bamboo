#pragma once

#include "collider_component.h"

namespace Bamboo
{
	class CapsuleColliderComponent : public ColliderComponent
	{
	public:
		CapsuleColliderComponent() { m_type = EColliderType::Capsule; }

		float m_radius = 0.5f;
		float m_height = 1.0f;

	private:
		REGISTER_REFLECTION(ColliderComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<ColliderComponent>(this)));
			ar(cereal::make_nvp("radius", m_radius));
			ar(cereal::make_nvp("height", m_height));
		}
	};
}