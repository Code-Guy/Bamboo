#pragma once

#include "light_component.h"

namespace Bamboo
{
	class DirectionalLightComponent : public LightComponent
	{
	public:

	private:
		REGISTER_REFLECTION(LightComponent)
		POLYMORPHIC_DECLARATION

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("light", cereal::base_class<LightComponent>(this)));
		}
	};
}