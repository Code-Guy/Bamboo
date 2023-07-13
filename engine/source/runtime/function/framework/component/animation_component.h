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
		REGISTER_COMPONENT

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("asset_ref", cereal::base_class<IAssetRef>(this)));
			bindRefs();
		}

		virtual void bindRefs() override;

		std::vector<std::shared_ptr<Animation>> m_animations;
	};
}