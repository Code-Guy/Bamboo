#include "vulkan_rhi.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/vulkan/vulkan_util.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/function/render/window_system.h"

#include <algorithm>

#define ENABLE_VALIDATION_LAYER DEBUG

namespace Bamboo
{
	void VulkanRHI::init()
	{
		createInstance();
#if ENABLE_VALIDATION_LAYER
		createDebugging();
#endif
		createSurface();
		pickPhysicalDevice();
		createLogicDevice();
		createVmaAllocator();

		createSwapchain();
		createSwapchainObjects();
		createCommandBuffers();
		createSynchronizationPrimitives();
	}

	void VulkanRHI::destroy()
	{
		for (VkFramebuffer framebuffer : m_framebuffers)
		{
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
		for (VkFence wait_fence : m_wait_fences)
		{
			vkDestroyFence(m_device, wait_fence, nullptr);
		}

		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		destroySwapchainObjects();
		vkDestroyCommandPool(m_device, m_command_pool, nullptr);

#if ENABLE_VALIDATION_LAYER
		destroyDebugging();
#endif
		vmaDestroyAllocator(m_vma_alloc);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyDevice(m_device, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	void VulkanRHI::resize()
	{
		// handle the window minimization corner case
		int width = 0;
		int height = 0;
		GLFWwindow* window = g_runtime_context.windowSystem()->getWindow();
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		// ensure all device operations have done
		vkDeviceWaitIdle(m_device);

		VkSwapchainKHR oldSwapchain = m_swapchain;
		createSwapchain();
		vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);

		destroySwapchainObjects();
		createSwapchainObjects();
	}

	void VulkanRHI::createInstance()
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

	void VulkanRHI::createDebugging()
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

	void VulkanRHI::destroyDebugging()
	{
		m_vk_destroy_debug_func(m_instance, m_debug_utils_messenger, nullptr);
	}

	void VulkanRHI::createSurface()
	{
		GLFWwindow* window = g_runtime_context.windowSystem()->getWindow();
		VkResult result = glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
		CHECK_VULKAN_RESULT(result, "create window surface");
	}

	void VulkanRHI::pickPhysicalDevice()
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

	void VulkanRHI::createLogicDevice()
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

	void VulkanRHI::createVmaAllocator()
	{
		VmaAllocatorCreateInfo vma_alloc_ci{};
		vma_alloc_ci.vulkanApiVersion = VK_API_VERSION_1_3;
		vma_alloc_ci.instance = m_instance;
		vma_alloc_ci.physicalDevice = m_physical_device;
		vma_alloc_ci.device = m_device;
		vma_alloc_ci.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;

		VkResult result = vmaCreateAllocator(&vma_alloc_ci, &m_vma_alloc);
		CHECK_VULKAN_RESULT(result, "create vma allocator");
	}

	void VulkanRHI::createSwapchain()
	{
		SwapchainSupportDetails swapchain_support_details = getSwapchainSupportDetails();
		m_surface_format = getProperSwapchainSurfaceFormat(swapchain_support_details);
		m_present_mode = getProperSwapchainSurfacePresentMode(swapchain_support_details);
		m_extent = getProperSwapchainSurfaceExtent(swapchain_support_details);
		VkImageUsageFlags image_usage = getProperSwapchainSurfaceImageUsage(swapchain_support_details);

		uint32_t image_count = std::min(swapchain_support_details.capabilities.minImageCount + 1, 
			swapchain_support_details.capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR swapchain_ci{};
		swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_ci.surface = m_surface;
		swapchain_ci.minImageCount = image_count;
		swapchain_ci.imageFormat = m_surface_format.format;
		swapchain_ci.imageColorSpace = m_surface_format.colorSpace;
		swapchain_ci.imageExtent = m_extent;
		swapchain_ci.imageArrayLayers = 1;
		swapchain_ci.imageUsage = image_usage;
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_ci.preTransform = swapchain_support_details.capabilities.currentTransform;
		swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_ci.presentMode = m_present_mode;
		swapchain_ci.clipped = VK_TRUE;
		swapchain_ci.oldSwapchain = m_swapchain;

		VkResult result = vkCreateSwapchainKHR(m_device, &swapchain_ci, nullptr, &m_swapchain);
		CHECK_VULKAN_RESULT(result, "create swapchain");
	}

	void VulkanRHI::destroySwapchainObjects()
	{
		// destroy swapchain image views
		for (VkImageView swapchain_image_view : m_swapchain_image_views)
		{
			vkDestroyImageView(m_device, swapchain_image_view, nullptr);
		}

		// destroy depth stencil image and view
		m_depth_stencil_image_view.destroy(m_device, m_vma_alloc);
	}

	void VulkanRHI::createSwapchainObjects()
	{
		// get swapchain images
		uint32_t last_swapchain_image_count = m_swapchain_image_count;
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchain_image_count, nullptr);
		ASSERT(last_swapchain_image_count == 0 || last_swapchain_image_count == m_swapchain_image_count,
			"swapchain image count shouldn't change");

		std::vector<VkImage> swapchain_images(m_swapchain_image_count);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchain_image_count, swapchain_images.data());

		// create swapchain image views
		m_swapchain_image_views.resize(m_swapchain_image_count);
		for (uint32_t i = 0; i < m_swapchain_image_count; ++i)
		{
			m_swapchain_image_views[i] = createImageView(m_device, swapchain_images[i], m_surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		// create depth stencil image and view
		std::vector<VkFormat> depth_format_candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		VkImageTiling depth_tiling = VK_IMAGE_TILING_OPTIMAL;
		VkFormatFeatureFlags depth_features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		VkImageUsageFlags depth_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkFormat depth_format = getProperImageFormat(depth_format_candidates, depth_tiling, depth_features);
		createImageAndView(m_device, m_vma_alloc, m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, depth_format, depth_tiling, depth_usage, 
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VK_IMAGE_ASPECT_DEPTH_BIT, m_depth_stencil_image_view);
	}

	void VulkanRHI::createCommandBuffers()
	{
		m_command_pool = createCommandPool(m_queue_family_indices.graphics);
		m_command_buffers.resize(m_swapchain_image_count);
		for (int i = 0; i < m_swapchain_image_count; ++i)
		{
			m_command_buffers[i] = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		}
	}

	void VulkanRHI::createSynchronizationPrimitives()
	{
		m_wait_fences.resize(m_swapchain_image_count);

		VkFenceCreateInfo fence_ci{};
		fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (VkFence& wait_fence : m_wait_fences)
		{
			vkCreateFence(m_device, &fence_ci, nullptr, &wait_fence);
		}
	}

	VkCommandPool VulkanRHI::createCommandPool(uint32_t queue_family_index, VkCommandPoolCreateFlags create_flags /*= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT*/)
	{
		VkCommandPoolCreateInfo command_pool_ci = {};
		command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_ci.queueFamilyIndex = queue_family_index;
		command_pool_ci.flags = create_flags;

		VkCommandPool command_pool;
		vkCreateCommandPool(m_device, &command_pool_ci, nullptr, &command_pool);
		return command_pool;
	}

	VkCommandBuffer VulkanRHI::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin /*= false*/)
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

	VkCommandBuffer VulkanRHI::createCommandBuffer(VkCommandBufferLevel level, bool begin /*= false*/)
	{
		return createCommandBuffer(level, m_command_pool, begin);
	}

	void VulkanRHI::flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, VkCommandPool pool, bool free /*= true*/)
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

	void VulkanRHI::flushCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue, bool free /*= true*/)
	{
		return flushCommandBuffer(command_buffer, queue, m_command_pool, free);
	}

	std::vector<const char*> VulkanRHI::getRequiredInstanceExtensions()
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

	std::vector<const char*> VulkanRHI::getRequiredInstanceLayers()
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

	std::vector<const char*> VulkanRHI::getRequiredDeviceExtensions()
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

	VkPhysicalDeviceFeatures VulkanRHI::getRequiredDeviceFeatures()
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

	VulkanRHI::QueueFamilyIndices VulkanRHI::getQueueFamilyIndices(std::vector<VkDeviceQueueCreateInfo>& queue_cis)
	{
		// get physical device queue family properties
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
		ASSERT(queue_family_count > 0, "no supported physical device queue family");
		m_queue_family_propertiess.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, m_queue_family_propertiess.data());

		// create device queue create infos
		VulkanRHI::QueueFamilyIndices queue_family_indices{};
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

			// ensure the graphic queue family must support presentation
			VkBool32 is_present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, queue_family_indices.graphics, m_surface, &is_present_support);
			ASSERT(is_present_support, "graphic queue family doesn't support presentation");
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

	uint32_t VulkanRHI::getQueueFamilyIndex(VkQueueFlags queue_flags)
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

	VulkanRHI::SwapchainSupportDetails VulkanRHI::getSwapchainSupportDetails()
	{
		VulkanRHI::SwapchainSupportDetails details;

		// surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &details.capabilities);

		// surface formats
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, nullptr);
		ASSERT(format_count > 0, "no supported surface formats");
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, details.formats.data());

		// surface present modes
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &present_mode_count, nullptr);
		ASSERT(present_mode_count > 0, "no supported surface present modes");
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &present_mode_count, details.present_modes.data());

		return details;
	}

	VkSurfaceFormatKHR VulkanRHI::getProperSwapchainSurfaceFormat(const SwapchainSupportDetails& details)
	{
		for (VkSurfaceFormatKHR format : details.formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}
		LOG_FATAL("no supported swapchain surface format: VK_FORMAT_B8G8R8A8_SRGB");
		return details.formats.front();
	}

	VkPresentModeKHR VulkanRHI::getProperSwapchainSurfacePresentMode(const SwapchainSupportDetails& details)
	{
		for (VkPresentModeKHR present_mode : details.present_modes)
		{
			if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return present_mode;
			}
		}
		LOG_FATAL("no supported swapchain surface present mode: VK_PRESENT_MODE_MAILBOX_KHR");
		return details.present_modes.front();
	}

	VkExtent2D VulkanRHI::getProperSwapchainSurfaceExtent(const SwapchainSupportDetails& details)
	{
		if (details.capabilities.currentExtent.width != UINT32_MAX)
		{
			return details.capabilities.currentExtent;
		}

		int width, height;
		GLFWwindow* window = g_runtime_context.windowSystem()->getWindow();
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actual_extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		actual_extent.width = std::clamp(actual_extent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);

		return actual_extent;
	}

	VkImageUsageFlags VulkanRHI::getProperSwapchainSurfaceImageUsage(const SwapchainSupportDetails& details)
	{
		ASSERT(details.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "swapchain doesn't support VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
		ASSERT(details.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT, "swapchain doesn't support VK_IMAGE_USAGE_TRANSFER_DST_BIT");
		return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VkFormat VulkanRHI::getProperImageFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties format_properties;
			vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &format_properties);

			if ((tiling == VK_IMAGE_TILING_LINEAR && (format_properties.linearTilingFeatures & features) == features) ||
				(tiling == VK_IMAGE_TILING_OPTIMAL && (format_properties.optimalTilingFeatures & features) == features))
			{
				return format;
			}
		}

		LOG_FATAL("failed to find a proper image format");
		return VK_FORMAT_UNDEFINED;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRHI::debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data)
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