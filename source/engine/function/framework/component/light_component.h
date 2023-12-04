#pragma once

#include "component.h"
#include "engine/core/color/color.h"

namespace Bamboo
{
	enum class ELightType
	{
		DirectionalLight, SkyLight, PointLight, SpotLight
	};

	class LightComponent : public Component
	{
	public:
		LightComponent();

		glm::vec3 getColor();

		float m_intensity;
		Color3 m_color;
		bool m_cast_shadow;

	private:
		REGISTER_REFLECTION(Component)
		POLYMORPHIC_DECLARATION

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("intensity", m_intensity));
			ar(cereal::make_nvp("color", m_color));
			ar(cereal::make_nvp("cast_shadow", m_cast_shadow));
		}
	};
}