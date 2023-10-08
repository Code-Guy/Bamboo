#pragma once

#include "engine/resource/asset/base/asset.h"

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
			ar(cereal::make_nvp("path_type", m_path_type));
			ar(cereal::make_nvp("bone_name", m_bone_name));
			ar(cereal::make_nvp("sampler_index", m_sampler_index));
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
			ar(cereal::make_nvp("interp_type", m_interp_type));
			ar(cereal::make_nvp("times", m_times)); 
			ar(cereal::make_nvp("values", m_values));
		}
	};

	class Animation : public Asset
	{
	public:
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
			ar(cereal::make_nvp("name", m_name));
			ar(cereal::make_nvp("samplers", m_samplers)); 
			ar(cereal::make_nvp("channels", m_channels));
		}
	};
}