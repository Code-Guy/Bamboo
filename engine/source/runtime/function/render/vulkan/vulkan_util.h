#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace Bamboo
{
	const char* vkErrorString(VkResult result);
	std::string vkPhysicalDeviceTypeString(VkPhysicalDeviceType type);
}