#pragma once

#include "component.h"

namespace Bamboo
{
	enum class EColliderType
	{
		Box, Sphere, Capsule, Cylinder, Mesh
	};

	class ColliderComponent : public Component
	{
	public:
		EColliderType m_type;

	private:
		REGISTER_REFLECTION(Component)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
		}
	};
}