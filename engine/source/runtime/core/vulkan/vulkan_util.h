#pragma once

#include "runtime/core/base/macro.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Bamboo
{
	// get VkResult error code string
	const char* vkErrorString(VkResult result);

	// get physical device type string from VkPhysicalDeviceType
	std::string vkPhysicalDeviceTypeString(VkPhysicalDeviceType type);

	// assert VkResult and log error code
#define CHECK_VULKAN_RESULT(result, msg) \
    if (result != 0) \
    { \
        LOG_FATAL("failed to {}, error: {}", msg, vkErrorString(result)); \
    }

	// begin and end instant transient commandbuffer
    VkCommandBuffer beginInstantCommands();
    void endInstantCommands(VkCommandBuffer command_buffer);

	// VMA Buffer
	struct VmaBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation;

		void destroy();
	};

	// VMA Image
	struct VmaImage
	{
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation;

		void destroy();
	};

	// VMA Image and Image view
	struct VmaImageView
	{
		VmaImage vma_image;
		VkImageView view = VK_NULL_HANDLE;

		void destroy();
		VkImage image() { return vma_image.image; }
	};

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaBuffer& buffer);
	void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

	void createImageAndView(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VkImageAspectFlags aspect_flags, VmaImageView& vma_image_view);
	void createImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VmaImage& vma_image);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
	VkSampler createSampler(VkFilter min_filter, VkFilter mag_filter, uint32_t mip_levels,
		VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w);

	void createVertexBuffer(uint32_t buffer_size, void* vertex_data, VmaBuffer& vertex_buffer);
	void createIndexBuffer(const std::vector<uint32_t>& indices, VmaBuffer& index_buffer);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createImageMipmaps(VkImage image, VkFormat image_format, uint32_t width, uint32_t height, uint32_t mip_levels);

	bool hasStencil(VkFormat format);
}