#include "box_collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::BoxColliderComponent>("BoxColliderComponent")
	.property("size", &Bamboo::BoxColliderComponent::m_size);
}

CEREAL_REGISTER_TYPE(Bamboo::BoxColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::BoxColliderComponent)

namespace Bamboo
{

}