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

	std::array<std::vector<char>*, 6> shaderCode;
	std::vector<Particle>* particles;
	std::vector<uint32_t>* offsets;
};


struct GravPushConstants {
	double deltaTime;
};

class GravEngine {
public:
	static const uint32_t COMPUTE_STEPS = 3;
	static const uint32_t GRID_CELL_COUNT = 32768;
	const uint32_t WORKSIZE = 1024;
	std::atomic_bool endTrigger = false;

	void initGrav_A(GravInit);
	void initGrav_B();
	void initMemory(std::array<MemInit, 4>);

	std::vector<std::string> shaderFiles = { "shaders/GravEngine/01.spv" , "shaders/GravEngine/SortShaders/01.spv", "shaders/GravEngine/SortShaders/02.spv", "shaders/GravEngine/SortShaders/03.spv", "shaders/GravEngine/SortShaders/04.spv", "shaders/GravEngine/SortShaders/05.spv" };
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

	uint32_t computeIndex = 0;

	void cleanup();

	void createRandomData();

	void runCommands();

	bool OLD_EX = false;

	void validateParticles();

	glm::ivec3 GRID_DIMENSIONS = { 32, 32, 32 };
	glm::vec3 DOMAIN_DIMENSIONS = { 16, 16, 16 };

private:
	VkDevice device;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::array<std::vector<char>*, 6> shaderCode;

	

	
	
	bool firstFrame = true;

	

	VkQueue gravQueue;

	std::vector<Particle>* particles;
	std::vector<uint32_t>* pOffsets;

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
	
	std::array<VkPipeline, 5>sortPipelines;
	VkPipelineLayout sortPipelineLayout;
	VkDescriptorSetLayout sortDescriptorSetLayout;
	std::array<VkDescriptorSet, COMPUTE_STEPS> sortDescriptorSets;
	
	std::array<std::array<VkBufferMemoryBarrier2, 6>, COMPUTE_STEPS> memoryBarriers;

	void createPipeline();
	void createDescriptorSets();
	void createCommandBuffers();
	void createStorageBuffers();
	void createSyncObjects();
	void createSortDescriptorSets();


	std::vector<uint32_t> deltaOffsets{};
	std::vector<uint32_t> offsets{};
	std::vector<uint32_t> newOffsets{};

	uint32_t frameIndex;

	void* mappedMem;
	void* mappedMem2;
	uint32_t* dOffMapped;
	uint32_t* offMapped;
	uint32_t* scanMapped;
	Particle* particlesMapped;

	std::vector<uint32_t> nullDOffData;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
	std::chrono::duration<double> renderDuration;

};