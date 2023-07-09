#include "animator_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::AnimatorComponent>("AnimatorComponent")
	 .constructor<>()
	 .property("m_skeleton", &Bamboo::AnimatorComponent::m_skeleton);
}

namespace Bamboo
{
	void AnimatorComponent::setSkeleton(std::shared_ptr<Skeleton>& skeleton)
	{
		m_skeleton = skeleton;
		m_ref_urls["m_skeleton"] = m_skeleton->getURL();
	}

	void AnimatorComponent::bindRefs()
	{
		if (m_skeleton)
		{
			return;
		}

		const auto& iter = m_ref_urls.begin();
		std::shared_ptr<Skeleton> skeleton = g_runtime_context.assetManager()->loadAsset<Skeleton>(iter->second);
		rttr::type::get(*this).get_property(iter->first).set_value(*this, skeleton);
	}

}