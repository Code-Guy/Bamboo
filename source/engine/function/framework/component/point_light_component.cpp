#include "point_light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::PointLightComponent>("PointLightComponent")
	 .property("radius", &Bamboo::PointLightComponent::m_radius)
	 .property("linear_attenuation", &Bamboo::PointLightComponent::m_linear_attenuation)
	 .property("quadratic_attenuation", &Bamboo::PointLightComponent::m_quadratic_attenuation);
}

CEREAL_REGISTER_TYPE(Bamboo::PointLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::LightComponent, Bamboo::PointLightComponent)

namespace Bamboo
{
	POLYMORPHIC_DEFINITION(PointLightComponent)

	PointLightComponent::PointLightComponent()
	{
		m_radius = 64.0f;
		m_linear_attenuation = 0.14f;
		m_quadratic_attenuation = 0.07f;
	}

}