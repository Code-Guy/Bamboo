#include "light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::LightComponent>("LightComponent")
	 .property("intensity", &Bamboo::LightComponent::m_intensity)
	 .property("color", &Bamboo::LightComponent::m_color)
	 .property("cast_shadow", &Bamboo::LightComponent::m_cast_shadow);
}

CEREAL_REGISTER_TYPE(Bamboo::LightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::LightComponent)

namespace Bamboo
{

	POLYMORPHIC_DEFINITION(LightComponent)

	LightComponent::LightComponent()
	{
		m_intensity = 1.0f;
		m_color = Color3::White;
		m_cast_shadow = true;
	}

	glm::vec3 LightComponent::getColor()
	{
		return (m_color * m_intensity).toVec3();
	}

}