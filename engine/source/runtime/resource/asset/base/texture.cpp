#include "texture.h"

namespace Bamboo
{

	Texture::Texture()
	{
		m_width = m_height = 0;
		m_min_filter = m_mag_filter = VK_FILTER_LINEAR;
		m_address_mode_u = m_address_mode_v = m_address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_mip_levels = 0;
		m_texture_type = ETextureType::Invalid;
	}

	Texture::~Texture()
	{
		m_image_view_sampler.destroy();
	}

	bool Texture::isSRGB()
	{
		return m_texture_type == ETextureType::BaseColor || m_texture_type == ETextureType::Emissive;
	}

}