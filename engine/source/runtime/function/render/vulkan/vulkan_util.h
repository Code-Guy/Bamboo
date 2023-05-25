#pragma once

#include "runtime/core/base/macro.h"
#include <vulkan/vulkan.h>

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
}