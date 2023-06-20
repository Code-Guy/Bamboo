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

	protected:
		bool isSRGB();

		VmaImageViewSampler m_image_view_sampler;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_width, m_height, m_min_filter, m_mag_filter,
				m_address_mode_u, m_address_mode_v, m_address_mode_w,
				m_mip_levels, m_texture_type);
		}
	};
}