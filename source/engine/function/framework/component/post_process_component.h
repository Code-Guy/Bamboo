#pragma once

#include "component.h"

namespace Bamboo
{
	class BloomFXComponent;

	class PostprocessComponent : public Component
	{
	public:
		PostprocessComponent();

	private:
		REGISTER_REFLECTION(Component)

	};

	class BloomFXComponent : public PostprocessComponent
	{
	public:
		BloomFXComponent();

		float m_intensity;
		float m_threshold;
	private:
		REGISTER_REFLECTION(PostprocessComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("bloom_fx_component", cereal::base_class<BloomFXComponent>(this)));
			ar(cereal::make_nvp("intensity", m_intensity));
			ar(cereal::make_nvp("threshold", m_threshold));
		}
	};
}