#define VMA_IMPLEMENTATION
#include "vulkan_util.h"

namespace Bamboo
{
	const char* vkErrorString(VkResult result)
	{
		switch (result)
		{
#define STR(r) case VK_ ##r: return #r
			STR(NOT_READY);
			STR(TIMEOUT);
			STR(EVENT_SET);
			STR(EVENT_RESET);
			STR(INCOMPLETE);
			STR(ERROR_OUT_OF_HOST_MEMORY);
			STR(ERROR_OUT_OF_DEVICE_MEMORY);
			STR(ERROR_INITIALIZATION_FAILED);
			STR(ERROR_DEVICE_LOST);
			STR(ERROR_MEMORY_MAP_FAILED);
			STR(ERROR_LAYER_NOT_PRESENT);
			STR(ERROR_EXTENSION_NOT_PRESENT);
			STR(ERROR_FEATURE_NOT_PRESENT);
			STR(ERROR_INCOMPATIBLE_DRIVER);
			STR(ERROR_TOO_MANY_OBJECTS);
			STR(ERROR_FORMAT_NOT_SUPPORTED);
			STR(ERROR_SURFACE_LOST_KHR);
			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			STR(SUBOPTIMAL_KHR);
			STR(ERROR_OUT_OF_DATE_KHR);
			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			STR(ERROR_VALIDATION_FAILED_EXT);
			STR(ERROR_INVALID_SHADER_NV);
#undef STR
		default:
			return "UNKNOWN_ERROR";
		}
	}

	std::string vkPhysicalDeviceTypeString(VkPhysicalDeviceType type)
	{
		switch (type)
		{
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
			STR(OTHER);
			STR(INTEGRATED_GPU);
			STR(DISCRETE_GPU);
			STR(VIRTUAL_GPU);
			STR(CPU);
#undef STR
		default: return "UNKNOWN_DEVICE_TYPE";
		}
	}


	VkCommandBuffer beginTransientCommandBuffer(VkDevice device, VkCommandPool command_pool)
	{
		VkCommandBufferAllocateInfo command_buffer_ai{};
		command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_ai.commandPool = command_pool;
		command_buffer_ai.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(device, &command_buffer_ai, &command_buffer);

		VkCommandBufferBeginInfo command_buffer_bi{};
		command_buffer_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &command_buffer_bi);

		return command_buffer;
	}

	void endTransientCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkCommandBuffer command_buffer)
	{
		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
	}

	void createImageAndView(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VkImageAspectFlags aspect_flags, VmaImageView& vma_image_view)
	{
		createImage(allocator, width, height, mip_levels, num_samples, format, tiling, image_usage, memory_usage, vma_image_view.vma_image);
		vma_image_view.view = createImageView(device, vma_image_view.vma_image.image, format, aspect_flags, mip_levels);
	}

	void createImage(VmaAllocator allocator, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags image_usage, VmaMemoryUsage memory_usage, VmaImage& image)
	{
		VkImageCreateInfo image_ci{};
		image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_ci.imageType = VK_IMAGE_TYPE_2D;
		image_ci.extent.width = width;
		image_ci.extent.height = height;
		image_ci.extent.depth = 1;
		image_ci.mipLevels = mip_levels;
		image_ci.arrayLayers = 1;
		image_ci.format = format;
		image_ci.tiling = tiling;
		image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_ci.usage = image_usage;
		image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_ci.samples = num_samples;
		image_ci.flags = 0;

		VmaAllocationCreateInfo vma_alloc_ci{};
		vma_alloc_ci.usage = memory_usage;

		image.mip_levels = mip_levels;
		VkResult result = vmaCreateImage(allocator, &image_ci, &vma_alloc_ci, &image.image, &image.allocation, nullptr);
		CHECK_VULKAN_RESULT(result, "vma create image");
	}

	VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
	{
		VkImageViewCreateInfo image_view_ci{};
		image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_ci.image = image;
		image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_ci.format = format;

		image_view_ci.subresourceRange.aspectMask = aspect_flags;
		image_view_ci.subresourceRange.baseMipLevel = 0;
		image_view_ci.subresourceRange.levelCount = mip_levels;
		image_view_ci.subresourceRange.baseArrayLayer = 0;
		image_view_ci.subresourceRange.layerCount = 1;

		VkImageView image_view;
		vkCreateImageView(device, &image_view_ci, nullptr, &image_view);

		return image_view;
	}
}