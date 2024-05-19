#pragma once

#include <vulkan/vulkan.h>

#include "geometry.h"
#include "structs.h"

class PhysicsEngine {
public:
	VkDevice device;
	VkCommandPool commandPool;
	VkCommandPool transferCommandPool;
	VkDescriptorPool descriptorPool;



	VkQueue gravQueue;
	VkQueue transferQueue;

	std::vector<VkCommandBuffer> gravCommandBuffers;
	std::vector<VkSemaphore> gravFinishedSemaphores;
	std::vector<VkFence> gravFinishedFences;
	std::vector<VkDescriptorSet> gravDescriptorSets;

	std::vector<VkBuffer> gravStorageBuffers;
	std::vector<VkDeviceMemory> gravStorageMemory;

	VkPipeline gravPipeline;
	VkPipelineLayout gravPipelineLayout;
	VkDescriptorSetLayout gravDescriptorSetLayout;
	

	
};