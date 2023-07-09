#pragma once

#include "component.h"
#include "runtime/resource/asset/skeleton.h"

namespace Bamboo
{
	class AnimatorComponent : public Component, public IAssetRef
	{
	public:
		void setSkeleton(std::shared_ptr<Skeleton>& skeleton);
		std::shared_ptr<Skeleton> getSkeleton() { return m_skeleton; }

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

		std::shared_ptr<Skeleton> m_skeleton;
	};
}

CEREAL_REGISTER_TYPE(Bamboo::AnimatorComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::AnimatorComponent)