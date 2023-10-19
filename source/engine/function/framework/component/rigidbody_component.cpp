#include "rigidbody_component.h"
#include "engine/core/base/macro.h"
#include "engine/function/physics/physics_system.h"
#include "engine/function/framework/entity/entity.h"
#include "engine/function/framework/component/transform_component.h"
#include "engine/function/framework/component/collider_component.h"

RTTR_REGISTRATION
{
rttr::registration::enumeration<Bamboo::EMotionType>("EMotionType")
(
	rttr::value("AlignLeft", Bamboo::EMotionType::Static),
	rttr::value("AlignRight", Bamboo::EMotionType::Kinematic),
	rttr::value("AlignHCenter", Bamboo::EMotionType::Dynamic)
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

	void RigidbodyComponent::beginPlay()
	{
		const auto& transform_component = getParent().lock()->getComponent(TransformComponent);
		const auto& collider_components = getParent().lock()->getChildComponents(ColliderComponent);
		if (!collider_components.empty())
		{
			m_body_id = g_engine.physicsSystem()->addRigidbody(transform_component->getGlobalMatrix(), this, collider_components);
		}
	}

	void RigidbodyComponent::endPlay()
	{
		if (m_body_id != UINT_MAX)
		{
			g_engine.physicsSystem()->removeRigidbody(m_body_id);
		}
	}

}