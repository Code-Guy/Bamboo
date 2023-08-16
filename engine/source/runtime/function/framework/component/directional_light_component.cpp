#include "directional_light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::DirectionalLightComponent>("DirectionalLightComponent");
}

CEREAL_REGISTER_TYPE(Bamboo::DirectionalLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::DirectionalLightComponent)

namespace Bamboo
{
	POLYMORPHIC_DEFINITION(DirectionalLightComponent)
}
