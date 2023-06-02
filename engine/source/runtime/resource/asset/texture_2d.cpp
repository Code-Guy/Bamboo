#include "texture_2d.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

#define IMAGE_COMPONENT 4
#define IMAGE_BIT_DEPTH 8

namespace Bamboo
{

	void Texture2D::loadFromGltf(const tinygltf::Image& gltf_image, const tinygltf::Sampler& gltf_sampler)
	{
		if (gltf_image.component != IMAGE_COMPONENT)
		{
			LOG_FATAL("unsupported gltf image component: {}", gltf_image.component);
		}
		if (gltf_image.bits != IMAGE_BIT_DEPTH)
		{
			LOG_FATAL("unsupported gltf image bit depth: {}", gltf_image.bits);
		}

		m_width = gltf_image.width;
		m_height = gltf_image.height;
		m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
		m_image_data = gltf_image.image;

		m_min_filter = getVkFilterFromGltf(gltf_sampler.minFilter);
		m_mag_filter = getVkFilterFromGltf(gltf_sampler.magFilter);
		m_address_mode_u = getVkAddressModeFromGltf(gltf_sampler.wrapS);
		m_address_mode_v = getVkAddressModeFromGltf(gltf_sampler.wrapT);
		m_address_mode_w = getVkAddressModeFromGltf(gltf_sampler.wrapR);

		inflate();
	}

	void Texture2D::inflate()
	{
		// create staging buffer
		VkDeviceSize image_size = m_image_data.size();
		VmaBuffer stagingBuffer;
		createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

		// copy image pixel data to staging buffer
		void* staging_buffer_data;
		vmaMapMemory(VulkanRHI::instance().getAllocator(), stagingBuffer.allocation, &staging_buffer_data);
		memcpy(staging_buffer_data, m_image_data.data(), image_size);
		vmaUnmapMemory(VulkanRHI::instance().getAllocator(), stagingBuffer.allocation);

		// create Image
		VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;
		createImageAndView(m_width, m_height, m_mip_levels, VK_SAMPLE_COUNT_1_BIT, image_format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			VK_IMAGE_ASPECT_COLOR_BIT, m_image_view);
		VkImage image = m_image_view.image();

		// transition image to DST_OPT state for copy into
		transitionImageLayout(image, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mip_levels);

		// copy staging buffer to image
		copyBufferToImage(stagingBuffer.buffer, image, m_width, m_height);

		// clear staging buffer
		vmaDestroyBuffer(VulkanRHI::instance().getAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);

		// generate image mipmaps, and transition image to READ_ONLY_OPT state for shader reading
		createImageMipmaps(image, image_format, m_width, m_height, m_mip_levels);

		// create VkSampler
		createSampler(m_min_filter, m_mag_filter, m_mip_levels, m_address_mode_u, m_address_mode_v, m_address_mode_w);
	}

	VkFilter Texture2D::getVkFilterFromGltf(int gltf_filter)
	{
		switch (gltf_filter) 
		{
		case -1:
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			return VK_FILTER_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return VK_FILTER_LINEAR;
		}

		return VK_FILTER_NEAREST;
	}

	VkSamplerAddressMode Texture2D::getVkAddressModeFromGltf(int gltf_wrap)
	{
		switch (gltf_wrap)
		{
		case -1:
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

}