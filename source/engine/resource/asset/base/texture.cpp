#include "texture.h"
#include <ktx.h>

namespace Bamboo
{

	Texture::Texture()
	{
		m_width = m_height = 0;
		m_min_filter = m_mag_filter = VK_FILTER_LINEAR;
		setAddressMode(VK_SAMPLER_ADDRESS_MODE_REPEAT);
		m_mip_levels = 0;
		m_texture_type = ETextureType::BaseColor;
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

	bool Texture::isMipmap()
	{
		return m_texture_type != ETextureType::Data;
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

	void Texture::uploadKtxTexture(void* p_ktx_texture, VkFormat format)
	{
		ktxTexture* ktx_texture = (ktxTexture*)p_ktx_texture;
		ktx_uint8_t* ktx_texture_data = ktxTexture_GetData(ktx_texture);
		ktx_size_t ktx_texture_size = ktxTexture_GetDataSize(ktx_texture);

		// create a staging buffer
		VmaBuffer staging_buffer;
		VulkanUtil::createBuffer(ktx_texture_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, staging_buffer);

		// copy image pixel data to staging buffer
		VulkanUtil::updateBuffer(staging_buffer, ktx_texture_data, ktx_texture_size);

		// create buffer image copy regions
		std::vector<VkBufferImageCopy> buffer_image_copies;
		for (uint32_t f = 0; f < m_layers; ++f)
		{
			for (uint32_t m = 0; m < m_mip_levels; ++m)
			{
				ktx_size_t offset;
				KTX_error_code result = ktxTexture_GetImageOffset(ktx_texture, m, 0, f, &offset);
				ASSERT(result == KTX_SUCCESS, "failed to get ktx image offset");

				VkBufferImageCopy buffer_image_copy{};
				buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				buffer_image_copy.imageSubresource.mipLevel = m;
				buffer_image_copy.imageSubresource.baseArrayLayer = f;
				buffer_image_copy.imageSubresource.layerCount = 1;
				buffer_image_copy.imageExtent.width = std::max(m_width >> m, 1u);
				buffer_image_copy.imageExtent.height = std::max(m_height >> m, 1u);
				buffer_image_copy.imageExtent.depth = 1;
				buffer_image_copy.bufferOffset = offset;

				buffer_image_copies.push_back(buffer_image_copy);
			}
		}

		// create vulkan texture
		format = format != VK_FORMAT_UNDEFINED ? format : getFormat();
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, m_mip_levels, m_layers, format,
			m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// transition texture to transfer dst optimal
		VkImage vk_image = m_image_view_sampler.image();
		VulkanUtil::transitionImageLayout(vk_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, format, m_mip_levels, m_layers);

		// copy staging buffer to texture
		VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();
		vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(buffer_image_copies.size()), buffer_image_copies.data());
		VulkanUtil::endInstantCommands(command_buffer);

		// transition texture to shader read only optimal
		VulkanUtil::transitionImageLayout(vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, format, m_mip_levels, m_layers);

		// clean up staging resources
		ktxTexture_Destroy(ktx_texture);
		staging_buffer.destroy();
	}

}