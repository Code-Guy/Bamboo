#include "directional_light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::DirectionalLightComponent>("DirectionalLightComponent")
	.property("cascade_frustum_near", &Bamboo::DirectionalLightComponent::m_cascade_frustum_near);
}

CEREAL_REGISTER_TYPE(Bamboo::DirectionalLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::DirectionalLightComponent)

namespace Bamboo
{
	POLYMORPHIC_DEFINITION(DirectionalLightComponent)

	DirectionalLightComponent::DirectionalLightComponent()
	{
		m_cascade_frustum_near = 0.0f;
	}

}
