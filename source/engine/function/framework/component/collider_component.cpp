#include "collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::ColliderComponent>("ColliderComponent")
	.property("position", &Bamboo::ColliderComponent::m_position)
	.property("rotation", &Bamboo::ColliderComponent::m_rotation);
}

CEREAL_REGISTER_TYPE(Bamboo::ColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::ColliderComponent)

namespace Bamboo
{
	POLYMORPHIC_DEFINITION(ColliderComponent)
}