#include "texture_2d.h"

CEREAL_REGISTER_TYPE(Bamboo::Texture2D)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Texture2D)
//
// #define KTX_CHECK(x)                                                                              \
// 	do                                                                                            \
// 	{                                                                                             \
// 		KTX_error_code err = x;                                                                   \
// 		if (err != KTX_SUCCESS)                                                                   \
// 		{                                                                                         \
// 			auto index = static_cast<uint32_t>(err);                                              \
// 			LOGE("Detected KTX error: {}", index < error_codes.size() ? error_codes[index] : ""); \
// 			abort();                                                                              \
// 		}                                                                                         \
// 	} while (0)

namespace Bamboo
{
	void Texture2D::inflate()
	{
		m_layers = 1;
		m_mip_levels = m_texture_type == ETextureType::BaseColor ? VulkanUtil::calcMipLevel(m_width, m_height) : 1;
		// todo 

		VulkanUtil::createImageViewSampler(m_width, m_height, m_image_data.data(), m_mip_levels, m_layers, getFormat(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
	}

	
}