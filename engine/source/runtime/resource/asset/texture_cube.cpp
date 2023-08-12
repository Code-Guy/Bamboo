#include "texture_cube.h"
#include <ktx.h>
#include <ktxvulkan.h>

CEREAL_REGISTER_TYPE(Bamboo::TextureCube)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::TextureCube)

namespace Bamboo
{
	void TextureCube::inflate()
	{	
		m_layers = 6;

		// create texture cube from ktx image data
		ktxTexture* ktx_texture;
		ktxResult result = ktxTexture_CreateFromMemory(m_image_data.data(), m_image_data.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
		ASSERT(result == KTX_SUCCESS, "failed to inflate texture cube");

		m_width = ktx_texture->baseWidth;
		m_height = ktx_texture->baseHeight;
		m_mip_levels = ktx_texture->numLevels;

		ktx_uint8_t* ktx_texture_data = ktxTexture_GetData(ktx_texture);
		ktx_size_t ktx_texture_size = ktxTexture_GetSize(ktx_texture);

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

				VkBufferImageCopy buffer_image_copy = {};
				buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				buffer_image_copy.imageSubresource.mipLevel = m;
				buffer_image_copy.imageSubresource.baseArrayLayer = f;
				buffer_image_copy.imageSubresource.layerCount = 1;
				buffer_image_copy.imageExtent.width = m_width >> m;
				buffer_image_copy.imageExtent.height = m_height >> m;
				buffer_image_copy.imageExtent.depth = 1;
				buffer_image_copy.bufferOffset = offset;

				buffer_image_copies.push_back(buffer_image_copy);
			}
		}

		// create vulkan texture cube
		VkFormat format = getFormat();
		VulkanUtil::createImageViewSampler(m_width, m_height, nullptr, m_mip_levels, m_layers, getFormat(),
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, m_image_view_sampler,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// transition texture cube to transfer dst optimal
		VkImage cube_image = m_image_view_sampler.image();
		VulkanUtil::transitionImageLayout(cube_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, format, m_mip_levels, m_layers);

		// copy staging buffer to texture cube
		VkCommandBuffer command_buffer = VulkanUtil::beginInstantCommands();
		vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, cube_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			static_cast<uint32_t>(buffer_image_copies.size()), buffer_image_copies.data());
		VulkanUtil::endInstantCommands(command_buffer);

		// transition texture cube to shader read only optimal
		VulkanUtil::transitionImageLayout(cube_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, format, m_mip_levels, m_layers);
	
		// clean up staging resources
		ktxTexture_Destroy(ktx_texture);
		staging_buffer.destroy();
	}
}