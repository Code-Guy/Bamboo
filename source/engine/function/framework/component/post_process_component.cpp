#include "post_process_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::BloomFXComponent>("BloomFXComponent")
	.property("intensity", &Bamboo::BloomFXComponent::m_intensity)
	.property("threshold", &Bamboo::BloomFXComponent::m_threshold);
}

CEREAL_REGISTER_TYPE(Bamboo::PostprocessComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::PostprocessComponent)
CEREAL_REGISTER_TYPE(Bamboo::BloomFXComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::PostprocessComponent, Bamboo::BloomFXComponent)

namespace Bamboo
{
	PostprocessComponent::PostprocessComponent()
	{

	}

	BloomFXComponent::BloomFXComponent()
	{
		m_intensity = 1.0f;
		m_threshold = -1.0f;
	}
}