#include "animation_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::AnimationComponent>("AnimationComponent")
	 .constructor<>()
	 .property("m_animation", &Bamboo::AnimationComponent::m_animation);
}

namespace Bamboo
{
	void AnimationComponent::setAnimation(std::shared_ptr<Animation>& animation)
	{
		m_animation = animation;
		m_ref_urls["m_animation"] = m_animation->getURL();
	}

	void AnimationComponent::bindRefs()
	{
		if (m_animation)
		{
			return;
		}

		const auto& iter = m_ref_urls.begin();
		std::shared_ptr<Animation> animation = g_runtime_context.assetManager()->loadAsset<Animation>(iter->second);
		rttr::type::get(*this).get_property(iter->first).set_value(*this, animation);
	}

}