#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <chrono>


#define CARD_COUNT 52

#include "structs.h"
#include "player.h"
#include "cardEngine.h"


struct CardInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass  renderPass;
	VkSampleCountFlagBits msaaSamples;

	VkPhysicalDeviceMemoryProperties memProperties;
	std::array<std::vector<char>*, 2> shaderCode;

	PlayerObject* player;

	GameTable gameTable;
	
};

struct CardPushConstants {
	float aspectRatio;
	glm::mat4 viewPojectionMatrix;
};
struct CardDataConstant {
	glm::mat4 cardMat;
};



class CardRasterizer {
public:
	void initCard_A(CardInit);
	void initCard_B();

	void initMemory(std::array<MemInit, 4>);
	
	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	void cleanup();

	uint32_t playerIndex = 0;

	static const uint32_t MAX_STRING_LENGTH = 4096;

	void drawElements(VkCommandBuffer, uint32_t, bool, glm::mat4);

	static const uint32_t FRAMES_IN_FLIGHT = 3;

	void initBufferData_A(MemoryDetails*);
	void initBufferData_B(VkCommandBuffer, VkQueue, MemInit);
	std::vector<std::string> shaderFiles = { "shaders/cardRasterizer/01.spv", "shaders/cardRasterizer/02.spv" };


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

	float* aspectRatio;

	VkBuffer vertexBuffer;
	MemInit vertexMemory;
	std::array < VkImage, 2>  texImage;
	std::array < VkImageView, 2>  texImageView;
	std::array < VkSampler, 2>  texSampler;
	std::array<MemInit, 2> texMemory;
	std::array<std::string, 2> texPaths = { "textures/Enhancers.png", "textures/8BitDeck.png" };
	std::array<VkDeviceSize, 2> texSizes;

	VkBuffer stagingBuffer;
	MemInit stagingMemory;


	VkBuffer uniformBuffer;
	MemInit uniformBufferMemory;
	char* uniformBufferMapped;
	uint32_t uniformBufferSize;
	uint32_t uniformBufferRegion;
	std::array<char*, FRAMES_IN_FLIGHT> uniformsMapped;

	std::vector<std::vector<playingCard>*>* hands;
	std::vector<std::vector<playingCard>*>* wins;
	std::vector<std::vector<playingCard>>* table;
	std::vector<playingCard>* stock;

	std::vector<glm::vec2> tablePositions{};
	glm::vec2 tableOffset;
	std::array<glm::vec2, 2> handPositons;
	std::array<glm::vec2, 4> handOffsets;
	glm::vec2 winOffset;
	std::array<glm::vec2, 2> winPositions;
	glm::vec2 stockOffset;
	glm::vec2 stockPosition;
	float cardHeightOffset;
	float cardHeightZero;

	PlayerObject* player;

	std::array<CardDataConstant, CARD_COUNT> cardData;
	std::array<glm::vec4, CARD_COUNT> currentCardData;
	std::array<glm::vec4, CARD_COUNT> prevCardData;

	std::chrono::time_point<std::chrono::high_resolution_clock> currentTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> commandSubmitTime = std::chrono::high_resolution_clock::now(); 

	uint32_t frameCounter = 0;
	uint32_t cardCounter = 0;
	
	std::array<std::vector<char>*, 2> shaderCode;

	MemoryDetails vertexRequirements{};
	MemoryDetails uniformRequirements{};
	std::array<MemoryDetails, 2> texRequirements{};

	
	void createPipeline();
	void createDescriptorSets();
	void createBuffers();
	void createImageView();
	void createSampler();




	std::vector<glm::vec4> vertices;


};
