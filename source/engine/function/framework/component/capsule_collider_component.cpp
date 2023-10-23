#include "capsule_collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::CapsuleColliderComponent>("CapsuleColliderComponent")
	.property("radius", &Bamboo::CapsuleColliderComponent::m_radius)
	.property("height", &Bamboo::CapsuleColliderComponent::m_height);
}

CEREAL_REGISTER_TYPE(Bamboo::CapsuleColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::ColliderComponent, Bamboo::CapsuleColliderComponent)

namespace Bamboo
{

	CapsuleColliderComponent::CapsuleColliderComponent()
	{
		m_type = EColliderType::Capsule;
		m_radius = 1.0f;
		m_height = 2.0f;
	}

}