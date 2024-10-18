#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "ft2build.h"
#include FT_FREETYPE_H



#include "structs.h"
#include "player.h"

struct UIInit {
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass  renderPass;

	VkPhysicalDeviceMemoryProperties memProperties;
	std::array<std::vector<char>*, 2> shaderCode;

	PlayerObject* player;

};

struct UIPushConstants {
	glm::vec2 charDimensions;
	glm::vec2 screenPosition;
	glm::vec2 screenDimensions;
	float  charAdvance;
	uint32_t renderStage;
};



class UIRasterizer {
public:
	void initUI_A(UIInit);
	void initUI_B();

	void initMemory(std::array<MemInit, 3>);
	
	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	void cleanup();

	uint32_t MAX_STRING_LENGTH = 256;

	void drawElements(VkCommandBuffer, uint32_t);

	static const uint32_t FRAMES_IN_FLIGHT = 3;

	void initBufferData_A(MemoryDetails*);
	void initBufferData_B(VkCommandBuffer, VkQueue, MemInit);
	std::vector<std::string> shaderFiles = { "shaders/uiRasterizer/01.spv", "shaders/uiRasterizer/02.spv" };


private:
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;


	VkBuffer vertexBuffer;
	MemInit vertexMemory;
	VkImage texImage;
	VkImageView texImageView;
	VkSampler texSampler;
	MemInit texMemory;

	VkBuffer stagingBuffer;
	MemInit stagingMemory;


	VkBuffer uniformBuffer;
	MemInit uniformBufferMemory;
	char* uniformBufferMapped;
	uint32_t uniformBufferSize;
	uint32_t uniformBufferRegion;
	std::array<char*, FRAMES_IN_FLIGHT> uniformsMapped;

	FT_Library library;
	FT_Face face;

	PlayerObject* player;

	std::vector<char*> bitmapData;
	uint32_t bitmapHeight;
	uint32_t bitmapWidth;
	uint32_t characterCount = 128;

	
	
	std::array<std::vector<char>*, 2> shaderCode;

	MemoryDetails vertexRequirements{};
	MemoryDetails uniformRequirements{};
	MemoryDetails texRequirements{};

	void createPipeline();
	void createDescriptorSets();
	void createBuffers();
	void createImageView();
	void createSampler();

	void initFreetype();

	std::vector<uint8_t> texPixels;
	uint16_t texWidth = 0;
	uint16_t texHeight = 0;
	uint16_t charWidth = 0;
	uint16_t charAdvance = 0;
	int16_t charStart = 32;
	int16_t charCount = 128 - charStart;
	int16_t stringLength = 256;

	std::vector<glm::vec2> vertices;


};
