#include "rigidbody_component.h"
#include "engine/core/base/macro.h"

RTTR_REGISTRATION
{
rttr::registration::enumeration<Bamboo::EMotionType>("EMotionType")
(
	rttr::value("Static", Bamboo::EMotionType::Static),
	rttr::value("Kinematic", Bamboo::EMotionType::Kinematic),
	rttr::value("Dynamic", Bamboo::EMotionType::Dynamic)
);

rttr::registration::class_<Bamboo::RigidbodyComponent>("RigidbodyComponent")
	.property("motion_type", &Bamboo::RigidbodyComponent::m_motion_type)
	.property("friction", &Bamboo::RigidbodyComponent::m_friction)
	.property("restitution", &Bamboo::RigidbodyComponent::m_restitution)
	.property("linear_damping", &Bamboo::RigidbodyComponent::m_linear_damping)
	.property("angular_damping", &Bamboo::RigidbodyComponent::m_angular_damping)
	.property("gravity_factor", &Bamboo::RigidbodyComponent::m_gravity_factor);
}

CEREAL_REGISTER_TYPE(Bamboo::RigidbodyComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::RigidbodyComponent)

namespace Bamboo
{
	POLYMORPHIC_DEFINITION(RigidbodyComponent)
}