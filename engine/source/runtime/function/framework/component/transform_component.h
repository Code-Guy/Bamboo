#pragma once

#include "component.h"
#include "runtime/core/math/transform.h"

namespace Bamboo
{
	class TransformComponent : public Component, public Transform
	{
	public:
		glm::mat4 local_matrix = glm::mat4(1.0f);
		glm::mat4 world_matrix = glm::mat4(1.0f);

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("position", m_position));
			ar(cereal::make_nvp("rotation", m_rotation));
			ar(cereal::make_nvp("scale", m_scale));
		}
	};
}

CEREAL_REGISTER_TYPE(Bamboo::TransformComponent);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::TransformComponent)