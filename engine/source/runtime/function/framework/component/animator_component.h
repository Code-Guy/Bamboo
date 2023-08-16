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
		REGISTER_REFLECTION(Component)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("asset_ref", cereal::base_class<IAssetRef>(this)));
		}

		virtual void bindRefs() override;

		std::shared_ptr<Skeleton> m_skeleton;
		Skeleton m_skeleton_inst;

		std::shared_ptr<class AnimationComponent> m_animation_component;
		std::shared_ptr<class SkeletalMeshComponent> m_skeletal_mesh_component;
		BoneUBO m_bone_ubo;

		float m_time = 0.0f;
		bool m_loop = true;
		bool m_playing = false;
		bool m_paused = false;
	};
}