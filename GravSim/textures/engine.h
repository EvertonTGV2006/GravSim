#pragma once





#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


#include <vector>
#include <iostream>
#include <set>
#include <optional>
#include <cstdlib>
#include <glm/glm.hpp>
#include <atomic>

#include "window.h"
#include "structs.h"
#include "player.h"


class VulkanEngine {
public:
	WindowManager winmanager;
	void initEngine();

	void cleanup();

	bool enableValidationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;

	OptionalSettings settings{ true };

	const int MAX_FRAMES_IN_FLIGHT = 2;
	const int MAX_COMPUTE_FRAMES = 3;


private:
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	std::vector<VkPhysicalDevice> devices;
	std::vector<int> deviceScores;
	int deviceScoreMax;
	VkPhysicalDeviceFeatures requiredDeviceFeatures{};
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;
	VkDescriptorPool descriptorPool;


	VkCommandPool graphicsCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;


	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	




	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	int isDeviceSuitable(VkPhysicalDevice);
	void createLogicalDevice();
	bool checkDeviceExtensionSupport(VkPhysicalDevice);
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createCommandPools();
	void createDepthResources();
	void createFramebuffers();
	void createDescriptorPool();



	void recreateSwapChain();
	void cleanupSwapChain();

	void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void createImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
	void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	void copyBufferToImage(VkBuffer, VkImage, uint32_t width, uint32_t height);

	uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
	VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
	QueueFamilyIndices findGraphicsQueueFamilies(VkPhysicalDevice);
	uint32_t findComputeQueueFamily(VkPhysicalDevice);
	uint32_t findTransferQueueFamily(VkPhysicalDevice);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME , VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME };



	uint32_t layerCount;
	std::vector<const char*> extensions;
	std::vector<const char*> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	bool checkValidationLayerSupport();
	std::vector<VkLayerProperties> availableLayers;
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};