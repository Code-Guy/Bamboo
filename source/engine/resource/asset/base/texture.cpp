#include "texture.h"

namespace Bamboo
{

	Texture::Texture()
	{
		m_width = m_height = 0;
		m_min_filter = m_mag_filter = VK_FILTER_LINEAR;
		setAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT);
		m_mip_levels = 0;
		m_texture_type = ETextureType::Invalid;
		m_pixel_type = EPixelType::RGBA8;
	}

	Texture::~Texture()
	{
		m_image_view_sampler.destroy();
	}

	void Texture::setAddressMode(VkSamplerAddressMode address_mode)
	{
		m_address_mode_u = m_address_mode_v = m_address_mode_w = address_mode;
	}

	bool Texture::isSRGB()
	{
		return m_texture_type == ETextureType::BaseColor || m_texture_type == ETextureType::Emissive;
	}

	VkFormat Texture::getFormat()
	{
		bool is_srgb = isSRGB();
		switch (m_pixel_type)
		{
		case EPixelType::RGBA8:
			return is_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
		case EPixelType::RGBA16:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case EPixelType::RGBA32:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case EPixelType::R16:
			return VK_FORMAT_R16_SFLOAT;
		case EPixelType::R32:
			return VK_FORMAT_R32_SFLOAT;
		case EPixelType::RG16:
			return VK_FORMAT_R16G16_SFLOAT;
		default:
			LOG_FATAL("unsupported pixel type: {}", m_pixel_type);
			return VK_FORMAT_R8G8B8A8_SRGB;
		}
	}

}