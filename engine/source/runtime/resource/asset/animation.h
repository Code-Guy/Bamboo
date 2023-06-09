#pragma once

#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	struct AnimationChannel
	{
		enum class EPathType 
		{ 
			Translation, Rotation, Scale 
		};

		EPathType m_path_type;
		std::string m_bone_name;
		uint32_t m_sampler_index;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_path_type, m_bone_name, m_sampler_index);
		}
	};

	struct AnimationSampler
	{
		enum class EInterpolationType
		{
			Linear, Step, CubicSpline
		};

		EInterpolationType m_interp_type;
		std::vector<float> m_times;
		std::vector<glm::vec4> m_values;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_interp_type, m_times, m_values);
		}
	};

	class Animation : public Asset
	{
	public:
		std::string m_name;
		std::vector<AnimationSampler> m_samplers;
		std::vector<AnimationChannel> m_channels;

		float m_start_time;
		float m_end_time;
		float m_duration;

		virtual void inflate() override;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_name, m_samplers, m_channels);
		}
	};
}