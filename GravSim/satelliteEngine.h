#pragma once

#include <array>
#include <vector>
#include <chrono>


#include "structs.h"

struct SatInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkCommandPool commandPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;

	std::array<std::vector <char>*,2> shaderCode;
};
struct SatPushConstants {
	float deltaTime;
};

class SatelliteEngine
{
public:
	static const uint32_t FRAMES_IN_FLIGHT = 3;
	static const uint32_t SATELLITE_COUNT = 1024;
	
	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;

	void initSatEngine_A(SatInit);
	void initSatEngine_B();
	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);
	void initMemory(std::array<MemInit, 3>);

	std::vector<std::string> shaderFiles = { "shaders/satelliteEngine/01.spv", "shaders/satelliteEngine/02.spv" };

	void simulateSats();

	MemoryDetails storageRequirements{};
	MemoryDetails uniformRequirements{};

	void cleanup();

private:
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkCommandPool commandPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline pipeline;
	VkPipeline linePipeline;
	VkPipelineLayout pipelineLayout;

	std::array<std::vector <char>*, 2> shaderCode;

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;
	VkBuffer lineBuffer;
	VkDeviceSize lineSize;
	MemInit lineMemory;

	std::array<VkBuffer, FRAMES_IN_FLIGHT> satBuffers;
	VkDeviceSize satSize;
	MemInit satMemory;

	VkBuffer uniformBuffer;
	MemInit uniformBufferMemory;
	VkDeviceSize uniformSize;
	std::array<char*, MAX_PLANET_ARRAY_SIZE> uniformBuffersMapped;

	void createPipeline();
	void createDescriptorSets();
	void createBuffers();
};

