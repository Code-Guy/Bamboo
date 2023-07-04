#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include <cereal/access.hpp>

enum class TextureType
{
	Invalid, BaseColor, MetallicRoughness, Normal, Occlusion, Emissive
};

namespace Bamboo
{
	class Texture
	{
	public:
		Texture();
		virtual ~Texture();

		uint32_t m_width, m_height;
		VkFilter m_min_filter, m_mag_filter;
		VkSamplerAddressMode m_address_mode_u, m_address_mode_v, m_address_mode_w;
		uint32_t m_mip_levels;
		TextureType m_texture_type;

		VmaImageViewSampler m_image_view_sampler;

	protected:
		bool isSRGB();

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("width", m_width));
			ar(cereal::make_nvp("height", m_height));
			ar(cereal::make_nvp("min_filter", m_min_filter));
			ar(cereal::make_nvp("mag_filter", m_mag_filter));
			ar(cereal::make_nvp("address_mode_u", m_address_mode_u));
			ar(cereal::make_nvp("address_mode_v", m_address_mode_v));
			ar(cereal::make_nvp("address_mode_w", m_address_mode_w));
			ar(cereal::make_nvp("mip_levels", m_mip_levels));
			ar(cereal::make_nvp("texture_type", m_texture_type));
		}
	};
}