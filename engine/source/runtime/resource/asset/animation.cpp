#include "animation.h"
#include <limits>

namespace Bamboo
{

	void Animation::inflate()
	{
		m_start_time = std::numeric_limits<float>::max();
		m_end_time = std::numeric_limits<float>::min();

		for (const AnimationSampler& sampler : m_samplers)
		{
			for (float time : sampler.m_times)
			{
				if (time < m_start_time)
				{
					m_start_time = time;
				}
				if (time > m_end_time)
				{
					m_end_time = time;
				}
			}
		}
		m_duration = m_end_time - m_start_time;
	}

}