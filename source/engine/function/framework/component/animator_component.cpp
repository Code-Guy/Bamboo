#include "animator_component.h"
#include "engine/function/global/engine_context.h"
#include "engine/resource/asset/asset_manager.h"
#include "engine/function/framework/entity/entity.h"
#include "engine/function/framework/component/animation_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::AnimatorComponent>("AnimatorComponent")
	 .property("skeleton", &Bamboo::AnimatorComponent::m_skeleton);
}

CEREAL_REGISTER_TYPE(Bamboo::AnimatorComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::AnimatorComponent)

namespace Bamboo
{

	AnimatorComponent::AnimatorComponent()
	{
		m_bone_ubs.resize(MAX_FRAMES_IN_FLIGHT);
		for (VmaBuffer& bone_ub : m_bone_ubs)
		{
			VulkanUtil::createBuffer(sizeof(BoneUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, bone_ub);
		}
	}

	AnimatorComponent::~AnimatorComponent()
	{
		for (VmaBuffer& bone_ub : m_bone_ubs)
		{
			bone_ub.destroy();
		}
	}

	void AnimatorComponent::setSkeleton(std::shared_ptr<Skeleton>& skeleton)
	{
		m_skeleton_inst = *skeleton;
		REF_ASSET(m_skeleton, skeleton)
	}

	void AnimatorComponent::inflate()
	{
		// tick once to update bone ubo
		tick(0.0f);
	}

	void AnimatorComponent::tick(float delta_time)
	{
		if (!m_animation_component)
		{
			m_animation_component = m_parent.lock()->getComponent(AnimationComponent);
		}

		if (!m_animation_component)
		{
			return;
		}

		const auto& animations = m_animation_component->getAnimations();
		if (animations.empty())
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
			Bone* bone = m_skeleton_inst.getBone(channel.m_bone_name);
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
					{
						LOG_FATAL("Unknown animation channel path type {}", channel.m_path_type);
					}
						break;
					}
				}
			}
		}

		// update time
		m_time += delta_time;
		if (m_loop && m_time > animation->m_end_time)
		{
			m_time -= animation->m_duration;
		}

		// update skeleton and bone matrices
		m_skeleton_inst.update();
		for (size_t i = 0; i < m_skeleton_inst.m_bones.size(); ++i)
		{
			m_bone_ubo.bone_matrices[i] = m_skeleton_inst.m_bones[i].matrix();
		}

		// update uniform buffers
 		for (VmaBuffer& bone_ub : m_bone_ubs)
 		{
 			VulkanUtil::updateBuffer(bone_ub, (void*)&m_bone_ubo, sizeof(BoneUBO));
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
		BIND_ASSET(m_skeleton, Skeleton)
		m_skeleton_inst = *m_skeleton;
	}

}