#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Bamboo
{
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