#pragma once

#include "collider_component.h"

namespace Bamboo
{
	class CylinderColliderComponent : public ColliderComponent
	{
	public:
		CylinderColliderComponent();

		float m_radius;
		float m_height;

	private:
		REGISTER_REFLECTION(ColliderComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("collider_component", cereal::base_class<ColliderComponent>(this)));
			ar(cereal::make_nvp("radius", m_radius));
			ar(cereal::make_nvp("height", m_height));
		}
	};
}