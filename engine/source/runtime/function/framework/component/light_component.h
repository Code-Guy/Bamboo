#pragma once

#include "component.h"

namespace Bamboo
{
	class LightComponent : public Component
	{
	public:
		float m_intensity;

	private:
		REGISTER_REFLECTION(Component)
		POLYMORPHIC_DECLARATION

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("intensity", m_intensity));
		}
	};
}