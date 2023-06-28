#include "texture_2d.h"

namespace Bamboo
{
	void Texture2D::inflate()
	{
		VulkanUtil::createImageViewSampler(m_width, m_height, m_image_data.data(), m_mip_levels, isSRGB(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
	}

}