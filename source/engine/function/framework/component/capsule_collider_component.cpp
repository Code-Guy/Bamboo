#include "capsule_collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::CapsuleColliderComponent>("CapsuleColliderComponent")
	.property("radius", &Bamboo::CapsuleColliderComponent::m_radius)
	.property("height", &Bamboo::CapsuleColliderComponent::m_height);
}

CEREAL_REGISTER_TYPE(Bamboo::CapsuleColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::CapsuleColliderComponent)

namespace Bamboo
{

}