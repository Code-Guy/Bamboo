#include "spot_light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SpotLightComponent>("SpotLightComponent")
	 .property("inner_cone_angle", &Bamboo::SpotLightComponent::m_inner_cone_angle)
	 .property("outer_cone_angle", &Bamboo::SpotLightComponent::m_outer_cone_angle);
}

CEREAL_REGISTER_TYPE(Bamboo::SpotLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::PointLightComponent, Bamboo::SpotLightComponent)

namespace Bamboo
{
	SpotLightComponent::SpotLightComponent()
	{
		m_inner_cone_angle = 30.0f;
		m_outer_cone_angle = 45.0f;
	}

}