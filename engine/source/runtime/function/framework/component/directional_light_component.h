#pragma once

#include "light_component.h"

namespace Bamboo
{
	class DirectionalLightComponent : public LightComponent
	{
	public:
		float m_cascade_frustum_near;

	private:
		REGISTER_REFLECTION(LightComponent)
		POLYMORPHIC_DECLARATION

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("light", cereal::base_class<LightComponent>(this)));
			ar(cereal::make_nvp("cascade_frustum_near", m_cascade_frustum_near));
		}
	};
}