#pragma once

#include <array>
#include <vector>
#include <chrono>


#include "structs.h"
#include "player.h"

struct SatInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	GlobalParameters* params;

	std::array<Planet, MAX_PLANET_ARRAY_SIZE>* planets;

	std::vector<std::vector <char>*> shaderCode;
};
struct SatPushConstants {
	double deltaTime;
	double elapsedTime;
	uint32_t targetPlanet;
	uint32_t planetIndex;
	
};
struct SatPlanetBuffer {
	std::array<std::array<Planet, MAX_PLANET_ARRAY_SIZE>, COMPUTE_STEPS_PER_FRAME * STEPS_PER_SHADER> planetData;
};
struct SatSpecConstants {
	double G;
	uint32_t MAX_PLANET_ARRAY_SIZE;
	uint32_t SATELLITE_COUNT;
	uint32_t LINE_VERTEX_COUNT;
	uint32_t COMPUTE_STEPS_PER_FRAME;
	uint32_t MESH_STEPS_MAJOR;
	uint32_t MESH_STEPS_MINOR;
	uint32_t SATELLITES_PER_SHADER;
	uint32_t STEPS_PER_SHADER;
	uint32_t MESH_PER_SHADER;
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
	void satelliteTransfer(VkCommandBuffer, VkQueue, bool);

	std::vector<std::string> shaderFiles = { "shaders/satelliteEngine/01.spv", "shaders/satelliteEngine/02.spv", "shaders/satelliteEngine/03.spv"};

	void simulateSats(VkCommandBuffer, uint32_t);
	void simulateSatsRaw(uint32_t);

	MemoryDetails lineRequirements{};
	MemoryDetails satRequirements{};
	MemoryDetails planetHostRequirements{};
	MemoryDetails lineInfoRequirements{};
	MemoryDetails fieldMeshRequirements{};
	MemoryDetails planetRequirements{};
	MemoryDetails satInfoRequirements{};
	MemoryDetails satTransferRequirements{};
	MemoryDetails fieldMeshIndexRequirements{};

	SatExternalMembers getSatellitePtrs();

	void cleanup();

private:
	VkDevice device;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline pipeline;
	VkPipeline linePipeline;
	VkPipeline meshPipeline;
	VkPipelineLayout pipelineLayout;

	GlobalParameters* params;
	std::array<std::vector <char>*, 3> shaderCode;

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT * 2> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;
	VkBuffer lineBuffer;
	VkDeviceSize lineSize;
	MemInit lineMemory;
	uint32_t lineCursor;
	uint32_t lineSegments;

	VkBuffer lineInfoBuffer;
	VkDeviceSize lineInfoSize;
	MemInit lineInfoMemory;

	std::array<VkBuffer, 2> satBuffers;
	VkDeviceSize satSize;
	MemInit satMemory;

	VkBuffer planetHostBuffer;
	MemInit planetHostMemory;
	VkDeviceSize planetSize;
	std::array<char*, FRAMES_IN_FLIGHT> planetBuffersMapped;

	VkBuffer satInfoBuffer;
	MemInit satInfoMemory;
	VkDeviceSize satInfoSize;

	VkBuffer planetBuffer;
	MemInit planetMemory;

	VkBuffer fieldMeshBuffer;
	VkBuffer fieldMeshIndexBuffer;
	MemInit fieldMeshMemory;
	MemInit fieldMeshIndexMemory;
	VkDeviceSize fieldMeshSize;
	VkDeviceSize fieldMeshIndexSize;

	glm::vec2 fieldMeshCorner1 = glm::vec2(-1e9f, -1e9f);
	glm::vec2 fieldMeshCorner2 = glm::vec2(1e9f, 1e9f);

	VkBuffer stagingBuffer;

	uint32_t lineFrame = 0;
	const uint32_t FRAMES_PER_LINE = 10;
	const uint32_t WRITE_FRAME = 0;

	std::array<Satellite, SATELLITE_COUNT>* satData;



	VkBuffer satTransferBuffer;
	MemInit satTransferMemory;
	VkDeviceSize satTransferSize;
	char* satTransferMapped;


	double elapsedTime;


	SatPlanetBuffer* satUBO = nullptr;

	void createPipeline();
	void createDescriptorSets();
	void createBuffers();

	void createInitialSatellitesBase(void*, uint32_t, uint32_t, double, double,double, double, uint32_t);
	void createInitialSatellitesOffset(void*, uint32_t, uint32_t, double, double, double, double, uint32_t);
	
	void updatePlanets(double);


	std::array<std::array<Planet, MAX_PLANET_ARRAY_SIZE>, 4> tempPlanets;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> tempAccelerations;

	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dx_1;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dv_1;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dx_2;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dv_2;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dx_3;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dv_3;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dx_4;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dv_4;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dx;
	std::array<glm::dvec3, MAX_PLANET_ARRAY_SIZE> dv;

	void updateAccelerations(uint32_t);

	void getOrbitalParams(Satellite*, uint32_t, Orbit*);



};

