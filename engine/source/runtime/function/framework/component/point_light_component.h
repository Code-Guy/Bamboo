#pragma once

#include "light_component.h"

namespace Bamboo
{
	class PointLightComponent : public LightComponent
	{
	public:
		float m_radius;
		float m_attenuation;

	private:
		REGISTER_REFLECTION(LightComponent)
		POLYMORPHIC_DECLARATION

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("light", cereal::base_class<LightComponent>(this)));
			ar(cereal::make_nvp("radius", m_radius));
			ar(cereal::make_nvp("attenuation", m_attenuation));
		}
	};
}