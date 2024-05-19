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


class VulkanRenderer {
public:
	void initVulkan();
	void drawFrame();
	void computeFrame();
	void cleanup();

	void queue();
	void queueDraw();
	void queueCompute();

	WindowManager winmanager;
	bool enableValidationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;

	std::vector<char> vertShaderCode;
	std::vector<char> fragShaderCode;
	std::vector<char> computeShaderCode;

	const double FPS_REFRESH_FREQUENCY = 1;


	OptionalSettings settings{ true };

	PlayerObject *player;

	bool threadingEnabled;

	int mode;

	const uint32_t ADD_PER_GROUP = 4;

	uint32_t longComputeCount=0;

	std::atomic_bool transferRequest;
	std::atomic_bool transferSubmitWait;
	std::atomic_bool transferFlag;
	std::atomic_uint32_t transferIndex;

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
	VkDescriptorSetLayout graphicsDescriptorSetLayout;
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool graphicsCommandPool;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;
	VkCommandPool computeCommandPool;
	VkCommandPool transferCommandPool;

	std::vector<VkBuffer> computeBuffers;
	std::vector<VkDeviceMemory> computeBufferMemory;
	
	std::vector<VkBuffer> transferBuffers;
	std::vector<VkDeviceMemory> transferBufferMemory;


	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> graphicsDescriptorSets;
	std::vector<VkDescriptorSet> computeDescriptorSets;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSempahores;
	std::vector<VkSemaphore> renderFinihsedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence>transferFinishedFences;

	std::vector<VkSemaphore> transferFinishedSemaphores;
	std::vector<VkSemaphore> vertexFinishedSemaphores;

	std::vector<VkFence> computeFences;
	std::vector<VkSemaphore> computeFinishedSemaphores1;
	std::vector<VkSemaphore> computeFinishedSemaphores2;
	std::vector<VkCommandBuffer> computeCommandBuffers;
	std::vector<VkCommandBuffer> transferCommandBuffers;

	static const int MAX_FRAMES_IN_FLIGHT = 2;
	static const int MAX_COMPUTE_FRAMES = 3;

	bool FirstFrame = true;

	std::atomic<bool> frameFlags[MAX_FRAMES_IN_FLIGHT];
	

	uint32_t currentFrame = 0;
	uint32_t frameCount = 0;
	

	uint32_t currentCompute = 0;
	std::atomic<uint32_t> computeCount = 0;
	double prevTime = glfwGetTime();
	double deltaTime;
	double deltaTimeCount;
	double computePrevTime = glfwGetTime();

	/*std::vector<glm::vec3> modelOffsets;*/
	std::vector<Particle> particles{ 
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
		{{0.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}}
	};

	const uint32_t WORKGROUP_SIZE = 32;

	



	std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	QueueFamilyIndices findGraphicsQueueFamilies(VkPhysicalDevice);
	uint32_t findComputeQueueFamily(VkPhysicalDevice);
	uint32_t findTransferQueueFamily(VkPhysicalDevice);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);
	VkShaderModule createShaderModule(const std::vector<char>&);



	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME , VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};

	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	int isDeviceSuitable(VkPhysicalDevice);
	void createLogicalDevice();
	bool checkDeviceExtensionSupport(VkPhysicalDevice);
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createComputePipeline();
	void createFramebuffers();
	void createCommandPools();
	void createDepthResources();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createComputeBuffers();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();

	void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void createImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags);
	void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	void copyBufferToImage(VkBuffer, VkImage, uint32_t width, uint32_t height);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);

	void updateUniformBuffer(uint32_t);
	void recordCommandBuffer(VkCommandBuffer, uint32_t);
	void recordTransferCommandBuffer(VkCommandBuffer, uint32_t, uint32_t);
	void recordComputeCommandBuffer(VkCommandBuffer, uint32_t);
	void recreateSwapChain();


	void cleanupSwapChain();

	uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);
	VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
	
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
