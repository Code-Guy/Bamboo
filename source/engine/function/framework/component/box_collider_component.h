#pragma once

#include "collider_component.h"

namespace Bamboo
{
	class BoxColliderComponent : public ColliderComponent
	{
	public:
		BoxColliderComponent() { m_type = EColliderType::Box; }

		glm::vec3 m_size = glm::vec3(1.0f);

	private:
		REGISTER_REFLECTION(ColliderComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<ColliderComponent>(this)));
			ar(cereal::make_nvp("size", m_size));
		}
	};
}