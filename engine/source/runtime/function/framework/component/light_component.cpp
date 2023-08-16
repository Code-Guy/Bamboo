#include "light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::LightComponent>("LightComponent")
	 .property("m_intensity", &Bamboo::LightComponent::m_intensity);
}

CEREAL_REGISTER_TYPE(Bamboo::LightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::LightComponent)

namespace Bamboo
{

	POLYMORPHIC_DEFINITION(LightComponent)

}