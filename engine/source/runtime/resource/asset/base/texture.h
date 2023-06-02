#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include <cereal/cereal.hpp>
#include <cereal/access.hpp>

namespace Bamboo
{
	class Texture
	{
	public:
		uint32_t m_width, m_height;
		VkFilter m_min_filter, m_mag_filter;
		VkSamplerAddressMode m_address_mode_u, m_address_mode_v, m_address_mode_w;
		uint32_t m_mip_levels;

		VmaImageView m_image_view;
		VkSampler m_sampler;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(m_width, m_height, m_min_filter, m_mag_filter,
				m_address_mode_u, m_address_mode_v, m_address_mode_w);
		}
	};
}