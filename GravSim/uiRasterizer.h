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


class UIRasterizer {
public:
	void initUI_A(UIInit);
	void initUI_B();

	void initMemory(std::array<MemInit, 3>);
	
	void getMemoryRequirements(std::vector<MemoryDetails>*, std::vector<uint16_t>*);

	void cleanup();

	void drawElements(VkCommandBuffer, uint32_t);

	static const uint32_t FRAMES_IN_FLIGHT = 3;

private:
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline boxPipeline;
	VkPipelineLayout boxPipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> boxDescriptorSets;
	VkDescriptorSetLayout boxDescriptorSetLayout;

	VkPipeline textPipeline;
	VkPipelineLayout textPipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> textDescriptorSets;
	VkDescriptorSetLayout textDescriptorSetLayouts;

	VkBuffer vertexBuffer;
	MemInit vertexMemory;

	VkBuffer uniformBuffer;
	MemInit uniformBufferMemory;
	char* uniformBufferMapped;
	uint32_t uniformBufferSize;

	FT_Library library;
	FT_Face face;

	PlayerObject* player;

	std::vector<char*> bitmapData;
	uint32_t bitmapHeight;
	uint32_t bitmapWidth;
	uint32_t characterCount = 128;

	
	std::array<std::vector<char>*, 2> shaderCode;

	void createPipelines();
	void createDescriptorSets();
	void createBuffers();

	void initFreetype();
	void initGlyphs();


};
