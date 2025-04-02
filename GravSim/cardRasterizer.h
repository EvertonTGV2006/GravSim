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
	VkRenderPass  renderPass;
	VkSampleCountFlagBits msaaSamples;

	VkPhysicalDeviceMemoryProperties memProperties;
	std::vector<std::vector<char>*> shaderCode;

	PlayerObject* player;
	DurakEngine* durak;
	uint32_t* locPlayerIndex;

	GlobalParameters* params;
	
};

struct CardPushConstants {
	glm::mat4 viewMat;
	glm::mat4 viewPojectionMatrix;
	glm::vec4 pos;
	glm::vec4 dir;
	glm::vec4 colour;
	glm::vec4 eyePos;
};




class CardRasterizer {
public:
	void initCard_A(CardInit);
	void initCard_B();
	StatusLogger* stat;

	void initMemory(MemInit*);
	
	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	void cleanup();

	DurakEngine* durak;
	uint32_t* locPlayerIndex = nullptr;

	GlobalParameters* params;
	float cardSize = 0.12f;

	uint32_t playerIndex = 0;

	void drawDurak(VkCommandBuffer, uint32_t, bool, glm::mat4, glm::mat4);

	void initBufferData_A(MemoryDetails*);
	void initBufferData_B(VkCommandBuffer, VkQueue, MemInit);
	std::vector<std::string> shaderFiles = { "shaders/cardRasterizer/03.spv", "shaders/cardRasterizer/04.spv" };

	void initDescriptors_A(std::vector<VkDescriptorPoolSize>*);
	void initDescriptors_B(VkDescriptorPool);


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

	VkRenderPass shadowPass;
	VkPipeline shadowPipeline;
	VkPipelineLayout shadowPipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> shadowDescriptorSets;
	VkDescriptorSetLayout shadowDescriptorSetLayout;

	VkImage depthImage;
	MemInit depthImageMemory;
	VkImageView depthView;
	VkFormat depthFormat = VK_FORMAT_D16_UNORM;

	VkImage shadowImage;
	MemInit shadowImageMemory;
	VkSampler shadowSampler;
	std::array<VkFramebuffer, 6> shadowFramebuffers;
	std::array<VkImageView, 6> shadowCubeViews;
	VkImageView shadowCubeMapView;
	VkFormat shadowFormat = VK_FORMAT_R32_SFLOAT;
	const uint32_t shadowImageSize = 1024;



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
	
	std::array<std::vector<char>*, 2> shaderCode;

	MemoryDetails vertexRequirements{};
	MemoryDetails uniformRequirements{};
	std::array<MemoryDetails, 2> texRequirements{};
	MemoryDetails depthRequirements{};
	MemoryDetails shadowRequirements{};

	
	void createPipeline();
	void createDescriptorSets();
	void createBuffers();
	void createImageView();
	void createSampler();



	void getCardData();
	uint32_t getCardMats();

	std::vector<glm::vec4> vertices;

	
	std::array<CardData, 64> cards_1;
	std::array<CardData, 64> cards_2;
	std::array<CardData, 64> cards_3;
	std::array<CardData, 64> cards_4;
	std::array<CardBuf, 64> cardMats;

	playingCard previousMouseCard{};
	playingCard currentMouseCard{};

	//animation data;
	std::chrono::time_point<std::chrono::high_resolution_clock> transitionAnimationStartTime = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> animationCurrentTime = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> mouseAnimationStartTime = std::chrono::high_resolution_clock::now();

	float smoothInterpolate(float, float, float);
};
