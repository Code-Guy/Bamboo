#include "animation_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::AnimationComponent>("AnimationComponent")
	 .property("m_animations", &Bamboo::AnimationComponent::m_animations);
}

CEREAL_REGISTER_TYPE(Bamboo::AnimationComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::AnimationComponent)

namespace Bamboo
{
	void AnimationComponent::addAnimation(std::shared_ptr<Animation>& animation)
	{
		m_animations.push_back({});
		uint32_t index = m_animations.size() - 1;
		REF_ASSET_ELEM(m_animations[index], std::to_string(index), animation)
	}

	void AnimationComponent::bindRefs()
	{
		for (auto iter : m_ref_urls)
		{
			m_animations.push_back({});
			uint32_t index = m_animations.size() - 1;
			BIND_ASSET_ELEM(m_animations[index], std::to_string(index), Animation)
		}
	}

}