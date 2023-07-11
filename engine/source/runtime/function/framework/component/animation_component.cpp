#include "animation_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::AnimationComponent>("AnimationComponent")
	 .constructor<>()
	 .property("m_animations", &Bamboo::AnimationComponent::m_animations);
}

CEREAL_REGISTER_TYPE(Bamboo::AnimationComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::AnimationComponent)

namespace Bamboo
{
	void AnimationComponent::addAnimation(std::shared_ptr<Animation>& animation)
	{
		m_ref_urls[std::to_string(m_animations.size())] = animation->getURL();
		m_animations.push_back(animation);
	}

	void AnimationComponent::bindRefs()
	{
		if (!m_animations.empty())
		{
			return;
		}

		for (auto iter : m_ref_urls)
		{
			std::shared_ptr<Animation> animation = g_runtime_context.assetManager()->loadAsset<Animation>(iter.second);
			m_animations.push_back(animation);
		}
	}

}