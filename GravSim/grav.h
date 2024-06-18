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

	std::array<std::vector<char>*, 4> shaderCode;
	std::vector<Particle>* particles;
};


struct GravPushConstants {
	double deltaTime;
};

class GravEngine {
public:
	static const uint32_t COMPUTE_STEPS = 3;
	static const uint32_t GRID_CELL_COUNT = 16 * 16 * 16;

	void initGrav_A(GravInit);
	void initGrav_B();
	void initMemory(std::array<MemInit, 4>);

	std::vector<std::string> shaderFiles = { "shaders/GravEngine/01.spv" , "shaders/GravEngine/SortShaders/01.spv", "shaders/GravEngine/SortShaders/02.spv", "shaders/GravEngine/SortShaders/03.spv" };
	//std::vector<std::string> shaderFiles = { "shaders/GravEngine/01.spv" };

	void syncBufferData_A(MemoryDetails*);
	void syncBufferData_B(bool, MemInit);

	std::array<VkSemaphore, COMPUTE_STEPS> getInterleavedSemaphores(std::array < VkSemaphore, COMPUTE_STEPS>);

	void simGrav(double);

	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	MemoryDetails storageRequirements{};
	MemoryDetails deltaRequirements{};
	MemoryDetails offsetRequirements{};
	MemoryDetails scanRequirements{};

	VkBuffer getInterleavedStorageBuffer();

	void cleanup();

	void createRandomData();

	void runCommands();

private:
	VkDevice device;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::array<std::vector<char>*, 4> shaderCode;

	
	
	bool firstFrame = true;

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

	VkBuffer deltaBuffer; //size no of cells / 2
	VkBuffer offsetBuffer; //size no of cells
	VkBuffer scanBuffer; //size no of cells / 1024

	MemInit deltaMem;
	MemInit offsetMem;
	MemInit scanMem;

	VkPipeline gravPipeline;
	VkPipelineLayout gravPipelineLayout;
	VkDescriptorSetLayout gravDescriptorSetLayout;
	
	std::array<VkPipeline, 3>sortPipelines;
	VkPipelineLayout sortPipelineLayout;
	VkDescriptorSetLayout sortDescriptorSetLayout;
	std::array<VkDescriptorSet, COMPUTE_STEPS> sortDescriptorSets;
	

	void createPipeline();
	void createDescriptorSets();
	void createCommandBuffers();
	void createStorageBuffers();
	void createSyncObjects();
	void createSortDescriptorSets();


	std::vector<uint32_t> deltaOffsets{};
	std::vector<uint32_t> offsets{};
	std::vector<uint32_t> newOffsets{};

};