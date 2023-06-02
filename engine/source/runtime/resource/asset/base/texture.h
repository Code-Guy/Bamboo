#pragma once

#include "runtime/core/vulkan/vulkan_util.h"

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
	};
}