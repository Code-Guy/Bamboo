#pragma once

#include "vulkan_util.h"

#include <functional>
#include <string>
#include <vector>
#include <map>

namespace Bamboo
{
	class VulkanRHI
	{
	public:
		void init();
		void waitFrame();
		void recordFrame();
		void submitFrame();
		void presentFrame();
		void destroy();

		void waitDeviceIdle() { vkDeviceWaitIdle(m_device); }

		const char* getVKAPIVersionStr();
		VkInstance getInstance() { return m_instance; }
		VkPhysicalDevice getPhysicalDevice() { return m_physical_device; }
		VkPhysicalDeviceProperties getPhysicalDeviceProperties() { return m_physical_device_properties; }
		VkFormat getColorFormat() { return m_surface_format.format; }
		VkFormat getDepthFormat() { return m_depth_format; }
		VkDevice getDevice() { return m_device; }
		uint32_t getGraphicsQueueFamily() { return m_queue_family_indices.graphics; }
		VkQueue getGraphicsQueue() { return m_graphics_queue; }
		VkQueue getTransferQueue() { return m_transfer_queue; }
		VmaAllocator getAllocator() { return m_allocator; }
		uint32_t getSwapchainImageCount() { return m_swapchain_image_count; }
		const std::vector<VkImageView>& getSwapchainImageViews() { return m_swapchain_image_views; }
		const VkExtent2D& getSwapchainImageSize() { return m_extent; }
		uint32_t getImageIndex() { return m_image_index; }
		uint32_t getFlightIndex() { return m_flight_index; }
		VkCommandPool getInstantCommandPool() { return m_instant_command_pool; }
		VkCommandBuffer getCommandBuffer() { return m_command_buffers[m_flight_index]; }
		PFN_vkCmdPushDescriptorSetKHR getVkCmdPushDescriptorSetKHR() { return m_vk_cmd_push_desc_set_func; }

		static VulkanRHI& get()
		{
			static VulkanRHI vulkan_rhi;
			return vulkan_rhi;
		}

	private:
		VulkanRHI() = default;
		~VulkanRHI() = default;

		struct QueueFamilyIndices
		{
			uint32_t graphics;
			uint32_t compute;
			uint32_t transfer;
		};

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> present_modes;
		};

		void createInstance();
		void createDebugging();
		void destroyDebugging();
		void createSurface();
		void pickPhysicalDevice();
		void validatePhysicalDevice();
		void createLogicDevice();
		void loadExtensionFuncs();
		void getDeviceQueues();
		void createVmaAllocator();
		void createSwapchain();
		void createSwapchainObjects();
		void destroySwapchainObjects();
		void recreateSwapchain();
		void createCommandPools();
		void createCommandBuffers();
		void createSynchronizationPrimitives();

		uint32_t getVKAPIVersion();
		std::vector<const char*> getRequiredInstanceExtensions();
		std::vector<const char*> getRequiredInstanceLayers();
		std::vector<const char*> getRequiredDeviceExtensions();
		VkPhysicalDeviceFeatures getRequiredDeviceFeatures();

		QueueFamilyIndices getQueueFamilyIndices(std::vector<VkDeviceQueueCreateInfo>& queue_cis);
		uint32_t getQueueFamilyIndex(VkQueueFlags queue_flags);

		SwapchainSupportDetails getSwapchainSupportDetails();
		VkSurfaceFormatKHR getProperSwapchainSurfaceFormat(const SwapchainSupportDetails& details);
		VkPresentModeKHR getProperSwapchainSurfacePresentMode(const SwapchainSupportDetails& details);
		VkExtent2D getProperSwapchainSurfaceExtent(const SwapchainSupportDetails& details);
		VkImageUsageFlags getProperSwapchainSurfaceImageUsage(const SwapchainSupportDetails& details);
		VkFormat getProperImageFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		bool isFormatSupported(VkFormat format);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		// vulkan objects
		VkInstance m_instance;
		VkPhysicalDevice m_physical_device;
		VkPhysicalDeviceProperties m_physical_device_properties;
		VkPhysicalDeviceFeatures m_physical_device_features;
		VkDevice m_device;
		VkQueue m_graphics_queue;
		VkQueue m_transfer_queue;
		VkSurfaceKHR m_surface;
		VmaAllocator m_allocator;
		VkCommandPool m_command_pool;
		VkCommandPool m_instant_command_pool;
		VkSwapchainKHR m_swapchain;

		// debug functions
		PFN_vkCreateDebugUtilsMessengerEXT m_vk_create_debug_func;
		PFN_vkDestroyDebugUtilsMessengerEXT m_vk_destroy_debug_func;
		VkDebugUtilsMessengerEXT m_debug_utils_messenger;

		// required extensions/layers/features of instance/device
		std::vector<const char*> m_required_instance_extensions;
		std::vector<const char*> m_required_instance_layers;
		std::vector<const char*> m_required_device_extensions;
		VkPhysicalDeviceFeatures m_required_device_features;

		// queue families
		QueueFamilyIndices m_queue_family_indices;
		std::vector<VkQueueFamilyProperties> m_queue_family_propertiess;

		// swapchain objects
		VkSurfaceFormatKHR m_surface_format;
		VkPresentModeKHR m_present_mode;
		VkExtent2D m_extent;
		VkFormat m_depth_format;

		uint32_t m_swapchain_image_count;
		std::vector<VkImageView> m_swapchain_image_views;

		// frame counters
		uint32_t m_flight_index;
		uint32_t m_image_index;
		uint32_t m_frame_index;

		// synchronization primitives
		std::vector<VkSemaphore> m_image_avaliable_semaphores;
		std::vector<VkSemaphore> m_render_finished_semaphores;
		std::vector<VkFence> m_flight_fences;
		std::vector<VkCommandBuffer> m_command_buffers;

		// additional device extension functions
		PFN_vkCmdPushDescriptorSetKHR m_vk_cmd_push_desc_set_func;
	};
}