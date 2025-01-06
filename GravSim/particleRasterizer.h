#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "structs.h"

struct RastInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkSampleCountFlagBits msaaSamples;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::vector<std::vector<char>*> shaderCode;
	std::vector<Mesh> meshes;

	uint32_t particleCount;
	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;
};

class particleRasterizer {
public:
	void initRast_A(RastInit);
	void initRast_B();

	void initMemory(MemInit*);
	void initBufferData_A(MemoryDetails*);
	void initBufferData_B(VkCommandBuffer, VkQueue, MemInit);

	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	MemoryDetails vertexRequirements{};
	MemoryDetails indexRequirements{};
	MemoryDetails lineRequirements{};

	MemoryDetails uniformRequirements{};
	
	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;

	void cleanup();

	void drawObjects(VkCommandBuffer, uint32_t, UniformBufferObject, float);

	static const uint32_t FRAMES_IN_FLIGHT = 3;

	std::vector<std::string> shaderFiles = { "shaders/particleRasterizer/01.spv", "shaders/particleRasterizer/02.spv", "shaders/particleRasterizer/03.spv", "shaders/particleRasterizer/04.spv" };
	void storeGravStorageBuffer(VkBuffer);

private:
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkSampleCountFlagBits msaaSamples;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPipeline linePipeline;

	std::vector<VkBuffer> vertexBuffers;
	std::vector<VkBuffer> indexBuffers;


	MemInit vertexMemory;
	MemInit indexMemory;
	uint32_t storageSize;
	std::vector<uint32_t> vertexOffsets;
	std::vector<uint32_t> indexOffsets;

	VkDeviceSize maxBufferSize = 0;

	
	std::vector<VkBuffer> lineBuffers;
	std::vector<uint32_t> lineBufferOffsets;
	MemInit lineMemory;
	std::vector<char*> lineBuffersMapped;


	uint32_t lineFrame = 0;
	std::vector<std::array<LineVertex, LINE_VERTEX_COUNT>> lineVertices;
	uint32_t lineCursor = 0;
	uint32_t lineSegments = 0;

	VkBuffer stagingBuffer;
	MemInit stagingMemory;
	
	VkBuffer uniformBuffer;
	MemInit uniformBufferMemory;
	char* uniformBufferMapped;
	uint32_t uniformBufferSize;

	std::array<std::vector<char>*, 4> shaderCode;

	std::vector<Mesh> meshes;

	uint32_t particleCount;

	void createPipeline();
	void createDescriptorSets();
	void createBuffers();

	
};