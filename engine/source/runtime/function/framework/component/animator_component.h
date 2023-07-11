#pragma once

#include "component.h"
#include "runtime/resource/asset/skeleton.h"
#include "host_device.h"

namespace Bamboo
{
	class AnimatorComponent : public Component, public IAssetRef
	{
	public:
		void setSkeleton(std::shared_ptr<Skeleton>& skeleton);
		std::shared_ptr<Skeleton> getSkeleton() { return m_skeleton; }

		virtual void tick(float delta_time) override;

		void play(bool loop = true);

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
		std::shared_ptr<class AnimationComponent> m_animation_component;
		std::shared_ptr<class SkeletalMeshComponent> m_skeletal_mesh_component;
		SkeletalMeshUBO m_skeletal_mesh_ubo;

		float m_time = 0.0f;
		bool m_loop = true;
		bool m_playing = false;
		bool m_paused = false;
	};
}