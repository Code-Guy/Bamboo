#include "vulkan_device.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/vulkan/vulkan_util.h"
#include <GLFW/glfw3.h>

#define ENABLE_VALIDATION_LAYER DEBUG

namespace Bamboo
{
	void VulkanDevice::init()
	{
		createInstance();
#if ENABLE_VALIDATION_LAYER
		createDebugging();
#endif
		pickPhysicalDevice();
		createLogicDevice();
		m_command_pool = createCommandPool(m_queue_family_indices.graphics);
	}

	void VulkanDevice::destroy()
	{
#if ENABLE_VALIDATION_LAYER
		destroyDebugging();
#endif
		
		vkDestroyCommandPool(m_device, m_command_pool, nullptr);
		vkDestroyDevice(m_device, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	void VulkanDevice::createInstance()
	{
		// set vulkan application info
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = APP_NAME;
		app_info.pEngineName = APP_NAME;
		app_info.apiVersion = VK_API_VERSION_1_3;
		app_info.applicationVersion = VK_MAKE_API_VERSION(0, APP_MAJOR_VERSION, APP_MINOR_VERSION, APP_PATCH_VERSION);
		app_info.engineVersion = app_info.applicationVersion;

		// set instance create info
		m_required_instance_extensions = getRequiredInstanceExtensions();
		m_required_instance_layers = getRequiredInstanceLayers();

		VkInstanceCreateInfo instance_ci{};
		instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_ci.pApplicationInfo = &app_info;
		instance_ci.enabledExtensionCount = static_cast<uint32_t>(m_required_instance_extensions.size());
		instance_ci.ppEnabledExtensionNames = m_required_instance_extensions.data();
		instance_ci.enabledLayerCount = static_cast<uint32_t>(m_required_instance_layers.size());
		instance_ci.ppEnabledLayerNames = m_required_instance_layers.data();

		// create instance
		VkResult result = vkCreateInstance(&instance_ci, nullptr, &m_instance);
		CHECK_VULKAN_RESULT(result, "create instance");
	}

	void VulkanDevice::createDebugging()
	{
		m_vk_create_debug_func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
		m_vk_destroy_debug_func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));

		VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_ci{};
		debug_utils_messenger_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_utils_messenger_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_utils_messenger_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		debug_utils_messenger_ci.pfnUserCallback = debugUtilsMessengerCallback;
		VkResult result = m_vk_create_debug_func(m_instance, &debug_utils_messenger_ci, nullptr, &m_debug_utils_messenger);
		CHECK_VULKAN_RESULT(result, "create debug utils messenger");
	}

	void VulkanDevice::destroyDebugging()
	{
		m_vk_destroy_debug_func(m_instance, m_debug_utils_messenger, nullptr);
	}

	void VulkanDevice::pickPhysicalDevice()
	{
		uint32_t gpu_count = 0;
		vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);
		ASSERT(gpu_count > 0, "failed to find a vulkan compatiable physical device");

		std::vector<VkPhysicalDevice> physical_devices(gpu_count);
		vkEnumeratePhysicalDevices(m_instance, &gpu_count, physical_devices.data());

		std::vector<VkPhysicalDevice> discrete_physical_devices;
		for (uint32_t i = 0; i < gpu_count; ++i)
		{
			VkPhysicalDeviceProperties physical_device_properties;
			vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties);
			LOG_INFO("device[{}]: {} {} {}.{}.{}", 
				i, physical_device_properties.deviceName, 
				vkPhysicalDeviceTypeString(physical_device_properties.deviceType),
				physical_device_properties.apiVersion >> 22,
				(physical_device_properties.apiVersion >> 12) & 0x3ff,
				physical_device_properties.apiVersion & 0xfff);

			// only use discrete gpu, for best performance
			if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				discrete_physical_devices.push_back(physical_devices[i]);
			}
		}

		// set the selected device index
		uint32_t selected_device_index = 0;
		if (selected_device_index >= discrete_physical_devices.size())
		{
			LOG_FATAL("selected device index {} is out of range {}", selected_device_index, discrete_physical_devices.size());
		}
		m_physical_device = discrete_physical_devices[selected_device_index];
	}

	void VulkanDevice::createLogicDevice()
	{
		m_required_device_extensions = getRequiredDeviceExtensions();
		m_required_device_features = getRequiredDeviceFeatures();
		std::vector<VkDeviceQueueCreateInfo> queue_cis;
		m_queue_family_indices = getQueueFamilyIndices(queue_cis);

		VkDeviceCreateInfo device_ci{};
		device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_ci.queueCreateInfoCount = static_cast<uint32_t>(queue_cis.size());
		device_ci.pQueueCreateInfos = queue_cis.data();
		device_ci.pEnabledFeatures = &m_required_device_features;
		device_ci.enabledExtensionCount = static_cast<uint32_t>(m_required_device_extensions.size());
		device_ci.ppEnabledExtensionNames = m_required_device_extensions.data();

		VkResult result = vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device);
		CHECK_VULKAN_RESULT(result, "create device");
	}

	VkCommandPool VulkanDevice::createCommandPool(uint32_t queue_family_index, VkCommandPoolCreateFlags create_flags /*= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT*/)
	{
		VkCommandPoolCreateInfo command_pool_ci = {};
		command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_ci.queueFamilyIndex = queue_family_index;
		command_pool_ci.flags = create_flags;

		VkCommandPool command_pool;
		vkCreateCommandPool(m_device, &command_pool_ci, nullptr, &command_pool);
		return command_pool;
	}

	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin /*= false*/)
	{
		VkCommandBufferAllocateInfo command_buffer_ai{};
		command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_ai.commandPool = pool;
		command_buffer_ai.level = level;
		command_buffer_ai.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(m_device, &command_buffer_ai, &command_buffer);

		if (begin)
		{
			VkCommandBufferBeginInfo command_buffer_bi{};
			command_buffer_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(command_buffer, &command_buffer_bi);
		}
		return command_buffer;
	}

	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin /*= false*/)
	{
		return createCommandBuffer(level, m_command_pool, begin);
	}

	void VulkanDevice::flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, VkCommandPool pool, bool free /*= true*/)
	{
		if (command_buffer == VK_NULL_HANDLE)
		{
			return;
		}

		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		VkFenceCreateInfo fence_ci{};
		fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence fence;
		vkCreateFence(m_device, &fence_ci, nullptr, &fence);

		vkQueueSubmit(queue, 1, &submit_info, fence);

		const uint64_t k_default_fence_time_out = 100000000000;
		vkWaitForFences(m_device, 1, &fence, VK_TRUE, k_default_fence_time_out);
		vkDestroyFence(m_device, fence, nullptr);

		if (free)
		{
			vkFreeCommandBuffers(m_device, pool, 1, &command_buffer);
		}
	}

	void VulkanDevice::flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free /*= true*/)
	{
		return flushCommandBuffer(command_buffer, queue, m_command_pool, free);
	}

	std::vector<const char*> VulkanDevice::getRequiredInstanceExtensions()
	{
		// find all supported instance extensions
		uint32_t supported_instance_extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &supported_instance_extension_count, nullptr);
		std::vector<VkExtensionProperties> supported_extension_propertiess(supported_instance_extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &supported_instance_extension_count, supported_extension_propertiess.data());
		std::vector<std::string> supported_instance_extensions;
		for (const VkExtensionProperties& extension_properties : supported_extension_propertiess)
		{
			supported_instance_extensions.push_back(extension_properties.extensionName);
		}

		// find glfw instance extensions
		uint32_t glfw_instance_extension_count = 0;
		const char** glfw_instance_extensions = glfwGetRequiredInstanceExtensions(&glfw_instance_extension_count);
		std::vector<const char*> required_instance_extensions(glfw_instance_extensions, glfw_instance_extensions + glfw_instance_extension_count);

		// if enable validation layer, add some extra debug extension
#if ENABLE_VALIDATION_LAYER
		required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		// add additional extensions
		// TODO

		// check if each required instance extension is supported
		for (const char* required_instance_extension : required_instance_extensions)
		{
			if (std::find(supported_instance_extensions.begin(), supported_instance_extensions.end(),
				required_instance_extension) == supported_instance_extensions.end())
			{
				LOG_FATAL("required instance extension {} is not supported", required_instance_extension);
			}
		}

		return required_instance_extensions;
	}

	std::vector<const char*> VulkanDevice::getRequiredInstanceLayers()
	{
		// find all supported instance layers
		uint32_t supported_instance_layer_count = 0;
		vkEnumerateInstanceLayerProperties(&supported_instance_layer_count, nullptr);
		std::vector<VkLayerProperties> supported_layer_propertiess(supported_instance_layer_count);
		vkEnumerateInstanceLayerProperties(&supported_instance_layer_count, supported_layer_propertiess.data());
		std::vector<std::string> supported_instance_layers;
		for (const VkLayerProperties& supported_layer_properties : supported_layer_propertiess)
		{
			supported_instance_layers.push_back(supported_layer_properties.layerName);
		}

		// set required instance layer
		std::vector<const char*> required_instance_layers;
#if ENABLE_VALIDATION_LAYER
		required_instance_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
	
		// check if each required instance layer is supported
		for (const char* required_instance_layer : required_instance_layers)
		{
			if (std::find(supported_instance_layers.begin(), supported_instance_layers.end(),
				required_instance_layer) == supported_instance_layers.end())
			{
				LOG_FATAL("required instance layer {} is not supported", required_instance_layer);
			}
		}

		return required_instance_layers;
	}

	std::vector<const char*> VulkanDevice::getRequiredDeviceExtensions()
	{
		// find all supported device extensions
		uint32_t supported_device_extension_count = 0;
		vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &supported_device_extension_count, nullptr);
		std::vector<VkExtensionProperties> supported_extension_propertiess(supported_device_extension_count);
		vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &supported_device_extension_count, supported_extension_propertiess.data());
		std::vector<std::string> supported_device_extensions;
		for (const VkExtensionProperties& extension_properties : supported_extension_propertiess)
		{
			supported_device_extensions.push_back(extension_properties.extensionName);
		}

		// set required device extensions
		std::vector<const char*> required_device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		// check if each required device extension is supported
		for (const char* required_device_extension : required_device_extensions)
		{
			if (std::find(supported_device_extensions.begin(), supported_device_extensions.end(),
				required_device_extension) == supported_device_extensions.end())
			{
				LOG_FATAL("required device extension {} is not supported", required_device_extension);
			}
		}

		return required_device_extensions;
	}

	VkPhysicalDeviceFeatures VulkanDevice::getRequiredDeviceFeatures()
	{
		VkPhysicalDeviceFeatures supported_device_features{};
		vkGetPhysicalDeviceFeatures(m_physical_device, &supported_device_features);

		// set required device features
		VkPhysicalDeviceFeatures required_device_features{};
		if (supported_device_features.sampleRateShading)
		{
			required_device_features.sampleRateShading = VK_TRUE;
		}

		if (supported_device_features.samplerAnisotropy)
		{
			required_device_features.samplerAnisotropy = VK_TRUE;
		}

		if (supported_device_features.geometryShader)
		{
			required_device_features.geometryShader = VK_TRUE;
		}

		if (supported_device_features.fillModeNonSolid)
		{
			required_device_features.fillModeNonSolid = VK_TRUE;
		}

		return required_device_features;
	}

	VulkanDevice::QueueFamilyIndices VulkanDevice::getQueueFamilyIndices(std::vector<VkDeviceQueueCreateInfo>& queue_cis)
	{
		// get physical device queue family properties
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
		ASSERT(queue_family_count > 0, "no supported physical device queue family");
		m_queue_family_propertiess.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, m_queue_family_propertiess.data());

		// create device queue create infos
		VulkanDevice::QueueFamilyIndices queue_family_indices{};
		VkQueueFlags required_queue_types = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
		const float k_default_queue_priority = 0.0f;
		queue_cis.clear();

		// graphics queue
		if (required_queue_types & VK_QUEUE_GRAPHICS_BIT)
		{
			queue_family_indices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
			VkDeviceQueueCreateInfo queue_ci{};
			queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_ci.queueFamilyIndex = queue_family_indices.graphics;
			queue_ci.queueCount = 1;
			queue_ci.pQueuePriorities = &k_default_queue_priority;
			queue_cis.push_back(queue_ci);
		}
		else
		{
			queue_family_indices.graphics = 0;
		}

		// dedicated compute queue
		if (required_queue_types & VK_QUEUE_COMPUTE_BIT)
		{
			queue_family_indices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
			if (queue_family_indices.compute != queue_family_indices.graphics)
			{
				// if compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queue_ci{};
				queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_ci.queueFamilyIndex = queue_family_indices.compute;
				queue_ci.queueCount = 1;
				queue_ci.pQueuePriorities = &k_default_queue_priority;
				queue_cis.push_back(queue_ci);
			}
		}
		else
		{
			// else we use the same queue
			queue_family_indices.compute = queue_family_indices.graphics;
		}

		// dedicated transfer queue
		if (required_queue_types & VK_QUEUE_TRANSFER_BIT)
		{
			queue_family_indices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
			if ((queue_family_indices.transfer != queue_family_indices.graphics) && (queue_family_indices.transfer != queue_family_indices.compute))
			{
				// if transfer family index differs, we need an additional queue create info for the transfer queue
				VkDeviceQueueCreateInfo queue_ci{};
				queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_ci.queueFamilyIndex = queue_family_indices.transfer;
				queue_ci.queueCount = 1;
				queue_ci.pQueuePriorities = &k_default_queue_priority;
				queue_cis.push_back(queue_ci);
			}
		}
		else
		{
			// else we use the same queue
			queue_family_indices.transfer = queue_family_indices.graphics;
		}

		return queue_family_indices;
	}

	uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlags queue_flags)
	{
		// find a queue only for compute, not for graphics
		if ((queue_flags & VK_QUEUE_COMPUTE_BIT) == queue_flags)
		{
			for (size_t i = 0; i < m_queue_family_propertiess.size(); ++i)
			{
				if ((m_queue_family_propertiess[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
					!(m_queue_family_propertiess[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					return i;
				}
			}
		}

		// find a queue only for transfer, not for graphics and compute
		if ((queue_flags & VK_QUEUE_TRANSFER_BIT) == queue_flags)
		{
			for (size_t i = 0; i < m_queue_family_propertiess.size(); ++i)
			{
				if ((m_queue_family_propertiess[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
					!(m_queue_family_propertiess[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(m_queue_family_propertiess[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
				{
					return i;
				}
			}
		}

		// for other queue types, return the first one that support the requested flags
		for (size_t i = 0; i < m_queue_family_propertiess.size(); ++i)
		{
			if ((m_queue_family_propertiess[i].queueFlags & queue_flags) == queue_flags)
			{
				return i;
			}
		}

		LOG_FATAL("failed to find a matching queue family index");
		return -1;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data)
	{
		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_WARNING("vulkan validation layer: {}", p_callback_data->pMessage);
		}
		else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ERROR("vulkan validation layer: {}", p_callback_data->pMessage);
		}

		return VK_FALSE;
	}
}