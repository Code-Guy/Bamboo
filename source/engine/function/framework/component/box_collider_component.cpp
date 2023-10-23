#include "box_collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::BoxColliderComponent>("BoxColliderComponent")
	.property("size", &Bamboo::BoxColliderComponent::m_size);
}

CEREAL_REGISTER_TYPE(Bamboo::BoxColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::ColliderComponent, Bamboo::BoxColliderComponent)

namespace Bamboo
{

	BoxColliderComponent::BoxColliderComponent()
	{
		m_type = EColliderType::Box;
		m_size = glm::vec3(1.0f);
	}

}