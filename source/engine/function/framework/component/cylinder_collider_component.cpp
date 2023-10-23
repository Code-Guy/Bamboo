#include "cylinder_collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::CylinderColliderComponent>("CylinderColliderComponent")
	.property("radius", &Bamboo::CylinderColliderComponent::m_radius)
	.property("height", &Bamboo::CylinderColliderComponent::m_height);
}

CEREAL_REGISTER_TYPE(Bamboo::CylinderColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::ColliderComponent, Bamboo::CylinderColliderComponent)

namespace Bamboo
{
	
	CylinderColliderComponent::CylinderColliderComponent()
	{
		m_type = EColliderType::Cylinder;
		m_radius = 1.0f;
		m_height = 2.0f;
	}

}