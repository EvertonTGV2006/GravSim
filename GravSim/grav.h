#pragma once

#include <vulkan/vulkan.h>

#include "geometry.h"
#include "structs.h"

#include <array>

struct GravInit {
	VkDevice device;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkQueue gravQueue;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::vector<char>* shaderCode;
	std::vector<Particle>* particles;
};


struct GravPushConstants {
	double deltaTime;
};

class GravEngine {
public:
	static const uint32_t COMPUTE_STEPS = 3;

	void initGrav(GravInit);
	void initMemory(MemInit);

	std::vector<std::string> shaderFiles = { "shaders/GravEngine/01.spv" };

	void syncBufferData_A(VkMemoryRequirements*);
	void syncBufferData_B(bool, MemInit);

	std::array<VkSemaphore, COMPUTE_STEPS> getInterleavedSemaphores(std::array < VkSemaphore, COMPUTE_STEPS>);

	void simGrav(double);

	void getMemoryRequirements(std::vector<VkMemoryRequirements>*, std::vector<uint16_t>*);

	VkMemoryRequirements storageRequirements{};

	VkBuffer getInterleavedStorageBuffer();

	void cleanup();

private:
	VkDevice device;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::vector<char>* shaderCode;

	
	
	bool firstFrame;

	uint32_t computeIndex = 0;

	VkQueue gravQueue;

	std::vector<Particle>* particles;

	VkCommandBuffer transferCommandBuffer;

	std::array<VkCommandBuffer, COMPUTE_STEPS> gravCommandBuffers;
	std::array<VkSemaphore, COMPUTE_STEPS> gravFinishedSemaphores;
	std::array<VkSemaphore, COMPUTE_STEPS> gravRenderSemaphores;
	std::array<VkSemaphore, COMPUTE_STEPS> renderGravSemaphores;
	std::array<VkFence, COMPUTE_STEPS> gravFinishedFences;
	std::array<VkDescriptorSet, COMPUTE_STEPS> gravDescriptorSets;

	VkBuffer storageBuffer;
	VkDeviceMemory storageMemory;
	uint32_t storageMemOffset;

	VkBuffer stagingBuffer;

	VkPipeline gravPipeline;
	VkPipelineLayout gravPipelineLayout;
	VkDescriptorSetLayout gravDescriptorSetLayout;
	

	

	void createPipeline();
	void createDescriptorSets();
	void createCommandBuffers();
	void createStorageBuffers();
	void createSyncObjects();

	
};