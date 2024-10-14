#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "structs.h"

class UI_Renderer {
public:
	static const uint32_t FRAMES_IN_FLIGHT = 3;



private:
	VkDevice device;
	VkDescriptorPool descriptorPool;
	VkRenderPass renderPass;

	VkPhysicalDeviceMemoryProperties memProperties;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;

	VkImage charImages;
	VkImageView charImageViews;
	MemInit imageMemory;
	
};