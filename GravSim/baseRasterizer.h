#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "structs.h"

struct RastInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::array<std::vector<char>*, 2> shaderCode;
	std::vector<Mesh> meshes;

	VkBuffer gravStorageBuffer;

	uint32_t particleCount;
};

class BaseRasterizer {
public:
	void initRast_A(RastInit);
	void initRast_B();

	void initMemory(std::array<MemInit, 3>);
	void initBufferData_A(MemoryDetails*);
	void initBufferData_B(VkCommandBuffer, VkQueue, MemInit);

	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	MemoryDetails vertexRequirements{};
	MemoryDetails indexRequirements{};

	MemoryDetails uniformRequirements{};
	

	void cleanup();

	void drawObjects(VkCommandBuffer, uint32_t, UniformBufferObject);

	static const uint32_t FRAMES_IN_FLIGHT = 3;

	std::vector<std::string> shaderFiles = { "shaders/baseRasterizer/01.spv", "shaders/baseRasterizer/02.spv" };
	void storeGravStorageBuffer(VkBuffer);

private:
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<VkBuffer> vertexBuffers;
	std::vector<VkBuffer> indexBuffers;
	MemInit vertexMemory;
	MemInit indexMemory;
	uint32_t storageSize;
	std::vector<uint32_t> vertexOffsets;
	std::vector<uint32_t> indexOffsets;

	VkDeviceSize maxBufferSize = 0;

	VkBuffer stagingBuffer;
	MemInit stagingMemory;
	
	VkBuffer uniformBuffer;
	MemInit uniformBufferMemory;
	char* uniformBufferMapped;
	uint32_t uniformBufferSize;

	VkBuffer gravStorageBuffer;


	std::array<std::vector<char>*, 2> shaderCode;

	std::vector<Mesh> meshes;

	uint32_t particleCount;

	void createPipeline();
	void createDescriptorSets();
	void createBuffers();

	
};