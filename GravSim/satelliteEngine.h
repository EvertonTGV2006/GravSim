#pragma once

#include <array>
#include <vector>
#include <chrono>


#include "structs.h"

struct SatInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;

	std::vector<std::vector <char>*> shaderCode;
};
struct SatPushConstants {
	float deltaTime;
};


class SatelliteEngine
{
public:
	static const uint32_t FRAMES_IN_FLIGHT = 3;

	
	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;

	void initSatEngine_A(SatInit);
	void initSatEngine_B();
	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);
	void initMemory(MemInit*);
	void initBufferData_A(MemoryDetails*);
	void initBufferData_B(VkCommandBuffer, VkQueue, MemInit);

	std::vector<std::string> shaderFiles = { "shaders/satelliteEngine/01.spv", "shaders/satelliteEngine/02.spv" };

	void simulateSats(VkCommandBuffer, uint32_t, float);

	MemoryDetails lineRequirements{};
	MemoryDetails satRequirements{};
	MemoryDetails uniformRequirements{};
	MemoryDetails lineInfoRequirements{};

	SatExternalMembers getSatellitePtrs();

	void cleanup();

private:
	VkDevice device;
	VkDescriptorPool descriptorPool;

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
	uint32_t lineCursor;
	uint32_t lineSegments;

	VkBuffer lineInfoBuffer;
	VkDeviceSize lineInfoSize;
	MemInit lineInfoMemory;

	std::array<VkBuffer, FRAMES_IN_FLIGHT> satBuffers;
	VkDeviceSize satSize;
	MemInit satMemory;

	VkBuffer uniformBuffer;
	MemInit uniformMemory;
	VkDeviceSize uniformSize;
	std::array<char*, FRAMES_IN_FLIGHT> uniformBuffersMapped;

	VkBuffer stagingBuffer;

	uint32_t lineFrame = 0;
	const uint32_t FRAMES_PER_LINE = 200;
	const uint32_t WRITE_FRAME = 0;

	std::array<Satellite, SATELLITE_COUNT> satData;

	void createPipeline();
	void createDescriptorSets();
	void createBuffers();

	void createInitialSatellites(void*, uint32_t, uint32_t, float, float,float, float, uint32_t);
	void updatePlanets(float);


	std::array<std::array<Planet, MAX_PLANET_ARRAY_SIZE>, 4> tempPlanets;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> tempAccelerations;

	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dx_1;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dv_1;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dx_2;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dv_2;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dx_3;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dv_3;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dx_4;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dv_4;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dx;
	std::array<glm::vec3, MAX_PLANET_ARRAY_SIZE> dv;

	void updateAccelerations(uint32_t);
};

