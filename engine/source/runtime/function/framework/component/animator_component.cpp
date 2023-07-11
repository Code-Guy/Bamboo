#include "animator_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/framework/entity/entity.h"
#include "runtime/function/framework/component/animation_component.h"
#include "runtime/function/framework/component/skeletal_mesh_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::AnimatorComponent>("AnimatorComponent")
	 .constructor<>()
	 .property("m_skeleton", &Bamboo::AnimatorComponent::m_skeleton);
}

CEREAL_REGISTER_TYPE(Bamboo::AnimatorComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::AnimatorComponent)

namespace Bamboo
{
	void AnimatorComponent::setSkeleton(std::shared_ptr<Skeleton>& skeleton)
	{
		m_skeleton = skeleton;
		m_ref_urls["m_skeleton"] = m_skeleton->getURL();
	}

	void AnimatorComponent::tick(float delta_time)
	{
		if (!m_animation_component)
		{
			m_animation_component = m_parent.lock()->getComponent<AnimationComponent>();
		}
		if (!m_skeletal_mesh_component)
		{
			m_skeletal_mesh_component = m_parent.lock()->getComponent<SkeletalMeshComponent>();
		}

		if (!m_animation_component || !m_skeletal_mesh_component)
		{
			return;
		}

		const auto& animations = m_animation_component->getAnimations();
		const auto& skeletal_mesh = m_skeletal_mesh_component->getSkeletalMesh();
		if (animations.empty() || !skeletal_mesh)
		{
			return;
		}

		const auto& animation = animations.front();
		if (m_time < animation->m_start_time)
		{
			m_time = animation->m_start_time;
		}

		// sampling animation
		for (const auto& channel : animation->m_channels)
		{
			Bone* bone = m_skeleton->getBone(channel.m_bone_name);
			const auto& sampler = animation->m_samplers[channel.m_sampler_index];

			for (size_t i = 0; i < sampler.m_times.size() - 1; ++i)
			{
				if (m_time >= sampler.m_times[i] && m_time <= sampler.m_times[i + 1])
				{
					float t = (m_time - sampler.m_times[i]) / (sampler.m_times[i + 1] - sampler.m_times[i]);
					switch (channel.m_path_type)
					{
					case AnimationChannel::EPathType::Translation:
					{
						glm::vec4 translation = glm::mix(sampler.m_values[i], sampler.m_values[i + 1], t);
						bone->setTranslation(translation);
					}
						break;
					case AnimationChannel::EPathType::Rotation:
					{
						glm::quat q0 = glm::make_quat(glm::value_ptr(sampler.m_values[i]));
						glm::quat q1 = glm::make_quat(glm::value_ptr(sampler.m_values[i + 1]));

						bone->setRotation(glm::slerp(q0, q1, t));
					}
						break;
					case AnimationChannel::EPathType::Scale:
					{
						glm::vec4 scale = glm::mix(sampler.m_values[i], sampler.m_values[i + 1], t);
						bone->setScale(scale);
					}
						break;
					default:
						break;
					}
				}
			}
		}

		// update time
		m_time += delta_time;
		if (m_loop && m_time > animation->m_end_time)
		{
			m_time = animation->m_start_time;
		}

		// update skeleton and bone matrices
		m_skeleton->update();
		for (size_t i = 0; i < m_skeleton->m_bones.size(); ++i)
		{
			m_skeletal_mesh_ubo.bone_matrices[i] = m_skeleton->m_bones[i].matrix();
		}

		// update uniform buffers
		for (VmaBuffer& uniform_buffer : skeletal_mesh->m_uniform_buffers)
		{
			VulkanUtil::updateBuffer(uniform_buffer, (void*)&m_skeletal_mesh_ubo, sizeof(SkeletalMeshUBO));
		}
	}

	void AnimatorComponent::play(bool loop)
	{
		m_loop = loop;
		m_playing = true;
		m_time = 0.0f;
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