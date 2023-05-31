#pragma once

#include "runtime/core/base/macro.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Bamboo
{
	const char* vkErrorString(VkResult result);
	std::string vkPhysicalDeviceTypeString(VkPhysicalDeviceType type);

#define CHECK_VULKAN_RESULT(result, msg) \
    if (result != 0) \
    { \
        LOG_FATAL("failed to {}, error: {}", msg, vkErrorString(result)); \
    }

    VkCommandBuffer beginTransientCommandBuffer(VkDevice device, VkCommandPool command_pool);
    void endTransientCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkCommandBuffer command_buffer);

	struct VmaImage
	{
		VkImage image;
		VmaAllocation allocation;
		uint32_t mip_levels;

		void destroy(VmaAllocator allocator)
		{
			vmaDestroyImage(allocator, image, allocation);
		}
	};

	struct VmaImageView
	{
		VmaImage vma_image;
		VkImageView view;

		void destroy(VkDevice device, VmaAllocator allocator)
		{
			vkDestroyImageView(device, view, nullptr);
			vma_image.destroy(allocator);
		}
	};

	void createImageAndView(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VkImageAspectFlags aspect_flags, VmaImageView& vma_image_view);

	void createImage(VmaAllocator allocator, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VmaImage& vma_image);
	VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
}