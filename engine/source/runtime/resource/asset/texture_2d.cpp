#include "texture_2d.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	Texture2D::Texture2D(const URL& url) : Asset(url)
	{
		m_asset_type = EAssetType::Texture2D;
		m_archive_type = EArchiveType::Json;
	}

	void Texture2D::inflate()
	{
		// create staging buffer
		VkDeviceSize image_size = m_image_data.size();
		VmaBuffer staging_buffer;
		createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, staging_buffer);

		// copy image pixel data to staging buffer
		void* staging_buffer_data;
		vmaMapMemory(VulkanRHI::instance().getAllocator(), staging_buffer.allocation, &staging_buffer_data);
		memcpy(staging_buffer_data, m_image_data.data(), image_size);
		vmaUnmapMemory(VulkanRHI::instance().getAllocator(), staging_buffer.allocation);

		// create Image
		VkFormat image_format = isSRGB() ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
		createImageAndView(m_width, m_height, m_mip_levels, VK_SAMPLE_COUNT_1_BIT, image_format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			VK_IMAGE_ASPECT_COLOR_BIT, m_image_view);
		VkImage image = m_image_view.image();

		// transition image to DST_OPT state for copy into
		transitionImageLayout(image, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mip_levels);

		// copy staging buffer to image
		copyBufferToImage(staging_buffer.buffer, image, m_width, m_height);

		// clear staging buffer
		vmaDestroyBuffer(VulkanRHI::instance().getAllocator(), staging_buffer.buffer, staging_buffer.allocation);

		// generate image mipmaps, and transition image to READ_ONLY_OPT state for shader reading
		createImageMipmaps(image, image_format, m_width, m_height, m_mip_levels);

		// create VkSampler
		m_sampler = createSampler(m_min_filter, m_mag_filter, m_mip_levels, m_address_mode_u, m_address_mode_v, m_address_mode_w);
	}

}