#pragma once

#include "light_component.h"

namespace Bamboo
{
	class PointLightComponent : public LightComponent
	{
	public:
		PointLightComponent();

		float m_radius;
		float m_linear_attenuation;
		float m_quadratic_attenuation;

	private:
		REGISTER_REFLECTION(LightComponent)
		POLYMORPHIC_DECLARATION

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("light", cereal::base_class<LightComponent>(this)));
			ar(cereal::make_nvp("radius", m_radius));
			ar(cereal::make_nvp("linear_attenuation", m_linear_attenuation));
			ar(cereal::make_nvp("quadratic_attenuation", m_quadratic_attenuation));
		}
	};
}