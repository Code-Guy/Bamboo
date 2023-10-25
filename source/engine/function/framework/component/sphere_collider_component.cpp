#include "sphere_collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SphereColliderComponent>("SphereColliderComponent")
	.property("radius", &Bamboo::SphereColliderComponent::m_radius);
}

CEREAL_REGISTER_TYPE(Bamboo::SphereColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::ColliderComponent, Bamboo::SphereColliderComponent)

namespace Bamboo
{

	SphereColliderComponent::SphereColliderComponent()
	{
		m_type = EColliderType::Sphere;
		m_radius = 1.0f;
	}

}