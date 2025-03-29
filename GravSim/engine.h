#pragma once





#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


#include <vector>
#include <iostream>
#include <set>
#include <optional>
#include <cstdlib>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <atomic>
#include <mutex>
#include <chrono>

#include "window.h"
#include "structs.h"
#include "player.h"

#include "uiRasterizer.h"
#include "cardRasterizer.h"
#include "networking.h"

class VulkanEngine {
public:
	WindowManager winmanager;
	PlayerObject* player;
	NetworkingClient nc;
	StatusLogger* stat;


	std::atomic_bool isDealer = false;
	std::atomic_bool isPlayerTurn = false;
	std::atomic_bool commandReady = false;
	std::mutex commandMutex;
	std::vector<char> commandString;
	std::string playerTurnString;
	std::string playerTurnStringEnd = "'s Turn";

	GlobalParameters params{};
	bool dGame = true;

	void initNetworking();
	
	void initEngine();

	void startDraw();

	void cleanup();

	bool enableValidationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;

	OptionalSettings settings{ true };

	uint32_t runNumber;
	uint32_t frameCounter = 0;
	uint32_t fpsVal = 0;
	std::array<double, 10> fpsAverage;
	uint32_t fpsIndex;

	bool lowPerformanceSetting;
	bool onlineGame;
	bool unlimitedFPS;

	int targetFrameTime_uS;

	float swapChainAspectRatio;

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

	std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> drawCommandBuffers;
	std::array<VkFence, FRAMES_IN_FLIGHT> flightFences;
	std::array<VkSemaphore, FRAMES_IN_FLIGHT> imageSemaphores;
	std::array<VkSemaphore, FRAMES_IN_FLIGHT> renderSemaphores;
	std::array<VkSemaphore, FRAMES_IN_FLIGHT> renderGravSemaphores; //render wait for grav
	std::array<VkSemaphore, FRAMES_IN_FLIGHT> gravRenderSemaphores;



	std::vector<std::chrono::duration<double>> frameTimes;

	VkCommandPool graphicsCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;

	VkImage colourImage;
	VkDeviceMemory colourImageMemory;
	VkImageView colourImageView;

	VkImage pickingImage;
	VkDeviceMemory pickingImageMemory;
	VkImageView pickingImageView;
	VkBuffer pickingBuffer;
	VkDeviceMemory pickingBufferMemory;
	VkFormat pickingFormat = VK_FORMAT_R8G8B8A8_UNORM;
	std::array<char*, FRAMES_IN_FLIGHT> pickingBuffersMapped;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	UIRasterizer uiRasterizer;
	CardRasterizer cardRasterizer;
	CardEngine cardEngine;
	DurakEngine durak;
	bool swapDealers = false;

	std::vector<MemInit> memoryContainers;
	std::vector<VkDeviceMemory> memory;

	VkPhysicalDeviceMemoryProperties memProperties{};

	uint32_t frameIndex = 0;
	
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_8_BIT;

	std::chrono::time_point<std::chrono::high_resolution_clock> pt = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> ct = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> dt;
	std::chrono::time_point<std::chrono::high_resolution_clock> nextFrameScheduled = std::chrono::high_resolution_clock::now();


	bool firstFrame = true;
	bool commandSubmitFrame = true;

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
	void createColourResources();
	void createDepthResources();
	void createPickingResources();
	void createFramebuffers();
	void createDescriptorPool();
	void createSyncObjects();
	void createCommandBuffers();

	void allocateMemory();
	void initSubclassData();

	void runCompute();
	void runGraphics();
	void executeCompute();
	void executeGraphics();
	void executeCardGame();
	void executeNetTasks();

	void recreateSwapChain();
	void cleanupSwapChain();

	void writeOutSampleData();

	void readFiles(std::vector<std::string>, std::vector<std::vector<char>>*);

	void createImage(uint32_t, uint32_t, VkSampleCountFlagBits, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);

	uint32_t findMemoryType(MemoryDetails);
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
		//std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		StatusLogger* statPtr = reinterpret_cast<StatusLogger*>(pUserData);
		std::ostringstream os;
		os << "Validation Layer: " << pCallbackData->messageIdNumber << " | " << pCallbackData->pMessage;
		statPtr->addMessage(MSG_LEVEL_GRAPHICS, os.str());
		
		return VK_FALSE;
	}
};