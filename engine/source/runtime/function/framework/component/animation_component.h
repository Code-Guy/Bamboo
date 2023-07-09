#pragma once

#include "component.h"
#include "runtime/resource/asset/animation.h"

namespace Bamboo
{
	class AnimationComponent : public Component, public IAssetRef
	{
	public:
		void addAnimation(std::shared_ptr<Animation>& animation);
		const std::vector<std::shared_ptr<Animation>>& getAnimations() { return m_animations; }

	private:
		RTTR_REGISTRATION_FRIEND
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("asset_ref", cereal::base_class<IAssetRef>(this)));
			bindRefs();
		}

		virtual void bindRefs() override;

		std::vector<std::shared_ptr<Animation>> m_animations;
	};
}

CEREAL_REGISTER_TYPE(Bamboo::AnimationComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::AnimationComponent)