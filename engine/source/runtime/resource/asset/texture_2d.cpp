#include "texture_2d.h"

namespace Bamboo
{
	void Texture2D::inflate()
	{
		m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
		VulkanUtil::createImageViewSampler(m_width, m_height, m_image_data.data(), m_mip_levels, isSRGB(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
	}

}