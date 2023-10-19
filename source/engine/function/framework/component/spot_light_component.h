#pragma once

#include "point_light_component.h"

namespace Bamboo
{
	class SpotLightComponent : public PointLightComponent
	{
	public:
		SpotLightComponent();

		float m_inner_cone_angle;
		float m_outer_cone_angle;

	private:
		REGISTER_REFLECTION(PointLightComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("point_light", cereal::base_class<PointLightComponent>(this)));
			ar(cereal::make_nvp("inner_cone_angle", m_inner_cone_angle));
			ar(cereal::make_nvp("outer_cone_angle", m_outer_cone_angle));
		}
	};
}