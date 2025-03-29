#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"

#include <fstream>
#include <charconv>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>

#include "cardRasterizer.h"
#include "structs.h"
#include "player.h"


void CardRasterizer::initCard_A(CardInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	renderPass = details.renderPass;

	msaaSamples = details.msaaSamples;

	memProperties = details.memProperties;
	memcpy(shaderCode.data(), details.shaderCode.data(), shaderCode.size() * sizeof(shaderCode[0]));

	player = details.player;
	durak = details.durak;
	params = details.params;
	locPlayerIndex = details.locPlayerIndex;

	//vertices = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(0, 1), glm::vec2(1, 0), glm::vec2(1, 1) };

	tablePositions = { {} };
	//front face counter clockwise 
	vertices = {
		{-0.5f, -0.5f, -0.5f, 0.0f},
		{-0.5f, 0.5f, -0.5f, 0.0f},
		{0.5f, -0.5f, -0.5f, 0.0f}, //triangle 1 bottom
		{-0.5f, 0.5f, -0.5f, 0.0f},
		{0.5f, 0.5f, -0.5f, 0.0f},
		{0.5f, -0.5f, -0.5f, 0.0f}, //traingle 2 bottom
		{-0.5f, -0.5f, 0.5f, 1.0f},
		{0.5f, -0.5f, 0.5f, 1.0f},
		{-0.5f, 0.5f, 0.5f, 1.0f}, //traingle 3 top
		{-0.5f, 0.5f, 0.5f, 1.0f},
		{0.5f, -0.5f, 0.5f, 1.0f},
		{0.5f, 0.5f, 0.5f, 1.0f} //triangle 4 top, do the other sides later
	};

	createBuffers();

}

void CardRasterizer::initCard_B() {
	createImageView();
	createSampler();
	createDescriptorSets();
	createPipeline();
}

void CardRasterizer::initMemory(MemInit* detPtr) {
	std::array<MemInit, 4> details;
	memcpy(details.data(), detPtr, details.size() * sizeof(MemInit));


	vertexMemory = details[0];
	uniformBufferMemory = details[1];
	for (uint32_t i = 0; i < texImage.size(); i++) {
		texMemory[i] = details[2 + i];
		vkBindImageMemory(device, texImage[i], texMemory[i].memory, texMemory[i].offset);
	}

	vkBindBufferMemory(device, vertexBuffer, vertexMemory.memory, vertexMemory.offset);
	vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory.memory, uniformBufferMemory.offset);


	void* data;
	vkMapMemory(device, uniformBufferMemory.memory, uniformBufferMemory.offset, uniformBufferMemory.range, 0, &data);

	uniformBufferMapped = static_cast<char*>(data);

	for (size_t i = 0; i < uniformsMapped.size(); i++) {
		uniformsMapped[i] = uniformBufferMapped + i * uniformBufferRegion;
	}

}

void CardRasterizer::initBufferData_A(MemoryDetails* stagingRequirements) {
	uint32_t maxSize = 0;
	for (uint32_t i = 0; i < texImage.size(); i++) {
		maxSize = std::max(maxSize, uint32_t(texSizes[i]));
	}
	maxSize = std::max(maxSize, uint32_t(sizeof(vertices[0]) * vertices.size()));
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = maxSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &createInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card staging Buffer"); }

	vkGetBufferMemoryRequirements(device, stagingBuffer, &(stagingRequirements->requirements));

	stagingRequirements->flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}
void CardRasterizer::initBufferData_B(VkCommandBuffer transferCommandBuffer, VkQueue transferQueue, MemInit memory){
	stagingMemory = memory;
	vkBindBufferMemory(device, stagingBuffer, stagingMemory.memory, stagingMemory.offset);

	//create neccessary sync objects
	VkFence transferFence;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(device, &fenceInfo, nullptr, &transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to create transfer fence for GravDataSync"); }

	//record copy operation
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	void* data;
	vkMapMemory(device, stagingMemory.memory, stagingMemory.offset, stagingMemory.range, 0, &data);

	size_t copySize = vertices.size() * sizeof(vertices[0]);
	memcpy(data, vertices.data(), copySize);
	VkBufferCopy copy{};
	copy.size = copySize;
	copy.srcOffset = 0;
	copy.dstOffset = 0;
	if (vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin transfer command buffer"); }
	vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, vertexBuffer, 1, &copy);
	if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to end transfer command buffer"); }
	vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &transferFence);
	vkResetCommandBuffer(transferCommandBuffer, 0);

	for (uint32_t i = 0; i < texImage.size(); i++) {
		int texWidth;
		int texHeight;
		int texChannels;


		stbi_uc* pixels = stbi_load(texPaths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		memcpy(data, pixels, texSizes[i]);
		stbi_image_free(pixels);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texImage[i];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		if (vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin transfer command buffer"); }

		vkCmdPipelineBarrier(transferCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { uint32_t(texWidth), uint32_t(texHeight), 1 };

		vkCmdCopyBufferToImage(transferCommandBuffer, stagingBuffer, texImage[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(transferCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to end transfer command buffer"); }
		vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
		vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &transferFence);
		vkResetCommandBuffer(transferCommandBuffer, 0);
	}
	vkUnmapMemory(device, memory.memory);

	vkDestroyFence(device, transferFence, nullptr);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void CardRasterizer::createBuffers() {
	//create texture atlas

	int texWidth;
	int texHeight;


	for (uint32_t i = 0; i < texImage.size(); i++) {

		if (stbi_info(texPaths[i].c_str(), &texWidth, &texHeight, nullptr) != 1) {
			throw std::runtime_error("Failed to load card texture");
		}


		VkDeviceSize imageSize = texWidth * texHeight * sizeof(uint8_t) * 4;
		texSizes[i] = imageSize;
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(texWidth);
		imageInfo.extent.height = static_cast<uint32_t>(texHeight);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		if (vkCreateImage(device, &imageInfo, nullptr, &texImage[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create texture atlas image"); }
	}
	uniformBufferRegion = sizeof(CardBuf) * cardMats.size();
	uniformBufferSize = uniformBufferRegion * FRAMES_IN_FLIGHT;

	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = sizeof(vertices[0]) * vertices.size();
	if (vkCreateBuffer(device, &createInfo, nullptr, &vertexBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card vertex buffer"); }
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo.size = uniformBufferSize;
	if (vkCreateBuffer(device, &createInfo, nullptr, &uniformBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card uniform buffer"); }



	VkImageCreateInfo depthImageInfo{};
	depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
	depthImageInfo.extent.width = shadowImageSize;
	depthImageInfo.extent.height = shadowImageSize;
	depthImageInfo.extent.depth = 1;
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;
	depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	depthImageInfo.format = depthFormat;
	depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	//if (vkCreateImage(device, &depthImageInfo, nullptr, &depthImage) != VK_SUCCESS) {throw std::runtime_error("Failed to create card depthImage");}
	VkImageCreateInfo shadowImageInfo{};
	shadowImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	shadowImageInfo.imageType = VK_IMAGE_TYPE_2D;
	shadowImageInfo.extent.width = shadowImageSize;
	shadowImageInfo.extent.height = shadowImageSize;
	shadowImageInfo.extent.depth = 1;
	shadowImageInfo.mipLevels = 1;
	shadowImageInfo.arrayLayers = 6;
	shadowImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	shadowImageInfo.format = depthFormat;
	shadowImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	shadowImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	shadowImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	shadowImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	shadowImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	//if (vkCreateImage(device, &shadowImageInfo, nullptr, &shadowImage) != VK_SUCCESS) {
	//	throw std::runtime_error("Failed to create card shadowImage");
	//}



	
	
	vkGetBufferMemoryRequirements(device, vertexBuffer, &vertexRequirements.requirements);
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements.requirements);
	//vkGetImageMemoryRequirements(device, depthImage, &depthRequirements.requirements);
	//vkGetImageMemoryRequirements(device, shadowImage, &shadowRequirements.requirements);

	for (uint32_t i = 0; i < texImage.size(); i++) {
		vkGetImageMemoryRequirements(device, texImage[i], &texRequirements[i].requirements);
		texRequirements[i].flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	depthRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	shadowRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vertexRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uniformRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


}
void CardRasterizer::createImageView() {
	for (uint32_t i = 0; i < texImage.size(); i++) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = texImage[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &texImageView[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI image view"); }
	}
	VkImageViewCreateInfo depthViewInfo{};
	depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthViewInfo.image = depthImage;
	depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthViewInfo.format = depthFormat;
	depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthViewInfo.subresourceRange.baseArrayLayer = 0;
	depthViewInfo.subresourceRange.layerCount = 1;
	depthViewInfo.subresourceRange.baseMipLevel = 0;
	depthViewInfo.subresourceRange.levelCount = 1;

	for (uint32_t i = 0; i < shadowCubeViews.size(); i++) {
		VkImageViewCreateInfo cubeInfo{};
		cubeInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		cubeInfo.image = shadowImage;
		cubeInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		cubeInfo.format = shadowFormat;
		cubeInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		cubeInfo.subresourceRange.baseArrayLayer = i;
		cubeInfo.subresourceRange.layerCount = 1;
		cubeInfo.subresourceRange.baseMipLevel = 0;
		cubeInfo.subresourceRange.levelCount = 1;
		//if (vkCreateImageView(device, &cubeInfo, nullptr, &shadowCubeViews[i]) != VK_SUCCESS) { throw std::runtime_error("Could not create shadow image view"); }
	}
	VkImageViewCreateInfo cubeInfo{};
	cubeInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	cubeInfo.image = shadowImage;
	cubeInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	cubeInfo.format = shadowFormat;
	cubeInfo.components = { VK_COMPONENT_SWIZZLE_R };
	cubeInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	cubeInfo.subresourceRange.baseArrayLayer = 0;
	cubeInfo.subresourceRange.layerCount = 6;
	cubeInfo.subresourceRange.baseMipLevel = 0;
	cubeInfo.subresourceRange.levelCount = 1;
	//if (vkCreateImageView(device, &cubeInfo, nullptr, &shadowCubeMapView) != VK_SUCCESS) { throw std::runtime_error("Could not create shadow image view"); }
	//if (vkCreateImageView(device, &depthViewInfo, nullptr, &depthView) != VK_SUCCESS) { throw std::runtime_error("Failed to create card depthImageView"); }

}
void CardRasterizer::createSampler() {
	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_NEAREST;
	createInfo.minFilter = VK_FILTER_NEAREST;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.anisotropyEnable = VK_FALSE;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	for (uint32_t i = 0; i < texImage.size(); i++) {
		if (vkCreateSampler(device, &createInfo, nullptr, &texSampler[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card sampler"); }
	}
	VkSamplerCreateInfo cubeInfo{};
	cubeInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	cubeInfo.magFilter = VK_FILTER_LINEAR;
	cubeInfo.minFilter = VK_FILTER_LINEAR;
	cubeInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	cubeInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	cubeInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	cubeInfo.anisotropyEnable = VK_FALSE;
	cubeInfo.unnormalizedCoordinates = VK_FALSE;
	cubeInfo.compareEnable = VK_FALSE;
	cubeInfo.compareOp = VK_COMPARE_OP_NEVER;
	cubeInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	cubeInfo.mipLodBias = 0.0f;
	cubeInfo.minLod = 0.0f;
	cubeInfo.maxLod = 1.0f;
	cubeInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	//if (vkCreateSampler(device, &createInfo, nullptr, &shadowSampler) != VK_SUCCESS) { throw std::runtime_error("Failed to create depth sampler"); }



}

void CardRasterizer::createDescriptorSets() {
	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 0;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = static_cast<uint32_t>(texImage.size());
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card descriptor set layout"); }

	std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { descriptorSetLayout, descriptorSetLayout, descriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate Card descriptor sets"); }



	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = i * uniformBufferRegion;
		bufferInfo.range = uniformBufferRegion;

		VkWriteDescriptorSet bufferWrite{};
		bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		bufferWrite.dstSet = descriptorSets[i];
		bufferWrite.dstBinding = 0;
		bufferWrite.dstArrayElement = 0;
		bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferWrite.descriptorCount = 1;
		bufferWrite.pBufferInfo = &bufferInfo;

		std::vector < VkDescriptorImageInfo> imageInfos{};
		std::vector<VkWriteDescriptorSet> descriptorWrites{};

		descriptorWrites.push_back(bufferWrite);


			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = texImageView[0];
			imageInfo.sampler = texSampler[0];
			imageInfos.push_back(imageInfo);

			VkWriteDescriptorSet imageWrite{};
			imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			imageWrite.dstSet = descriptorSets[i];
			imageWrite.dstBinding = 1;
			imageWrite.dstArrayElement = 0;
			imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageWrite.descriptorCount = 1;
			imageWrite.pImageInfo = &imageInfo;
			descriptorWrites.push_back(imageWrite);

			VkDescriptorImageInfo imageInfo2{};
			imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo2.imageView = texImageView[1];
			imageInfo2.sampler = texSampler[1];
			imageInfos.push_back(imageInfo2);

			VkWriteDescriptorSet imageWrite2{};
			imageWrite2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			imageWrite2.dstSet = descriptorSets[i];
			imageWrite2.dstBinding = 1;
			imageWrite2.dstArrayElement = 1;
			imageWrite2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageWrite2.descriptorCount = 1;
			imageWrite2.pImageInfo = &imageInfo2;
			descriptorWrites.push_back(imageWrite2);





		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void CardRasterizer::createPipeline() {

	std::array<VkShaderModule, 2> shaderModules{};
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	std::array<VkShaderStageFlagBits, 2> flagBits{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };


	for (uint32_t i = 0; i < shaderCode.size(); i++) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode[i]->size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode[i]->data());
		createInfo.pNext = nullptr;

		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModules[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizerShaderModule"); }

		shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[i].stage = flagBits[i];
		shaderStages[i].module = shaderModules[i];
		shaderStages[i].pName = "main";
		shaderStages[i].pNext = nullptr;
		shaderStages[i].pSpecializationInfo = nullptr;
		shaderStages[i].flags = 0;
	}
	VkVertexInputAttributeDescription attributeDescription{};
	attributeDescription.binding = 0;
	attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescription.location = 0;
	attributeDescription.offset = 0;

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindingDescription.stride = static_cast<uint32_t>(sizeof(vertices[0]));

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //for point rendering
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = (params->multisampling) ? msaaSamples : VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;


	std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments = { colorBlendAttachment, colorBlendAttachment };

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;


	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPushConstantRange constantRange{};
	constantRange.size = sizeof(CardPushConstants);
	constantRange.offset = 0;
	constantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;


	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &constantRange;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card pipeline layout"); }
	VkPipelineShaderStageCreateInfo shaderStages2[] = { shaderStages[0], shaderStages[1] };

	VkGraphicsPipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	createInfo.pStages = shaderStages.data();
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pMultisampleState = &multisampling;
	createInfo.pColorBlendState = &colorBlending;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = pipelineLayout;
	createInfo.renderPass = renderPass;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.subpass = 0;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create Card pipeline"); }

	for (uint32_t i = 0; i < shaderModules.size(); i++) {
		vkDestroyShaderModule(device, shaderModules[i], nullptr);
	}
}

void CardRasterizer::getMemoryRequirements(std::vector<MemoryDetails>* details, std::vector<uint16_t>* count) {
	count->push_back(2 + static_cast<uint16_t>(texRequirements.size()));
	details->push_back(vertexRequirements);
	details->push_back(uniformRequirements);
	for (uint32_t i = 0; i < texImage.size(); i++) {
		details->push_back(texRequirements[i]);
	}
}


void CardRasterizer::drawDurak(VkCommandBuffer commandBuffer, uint32_t frameIndex, bool cmdFrame, glm::mat4 viewMat, glm::mat4 projMat) {
	
	if (cmdFrame) {
		params->commandAnimationQueued = true;
	}
	if (params->commandAnimationQueued && !params->transitionAnimationPlaying && !params->mouseAnimationPlaying) {
		cards_1 = cards_2;
		getCardData();
		transitionAnimationStartTime = std::chrono::high_resolution_clock::now();
		params->transitionAnimationPlaying = true;
		params->commandAnimationQueued = false;
	}
	//cards0 is 2nd previous positions
	//cards1 is previous positions
	//cards2 is is current postions
	//cards3 is desired positions;
	animationCurrentTime = std::chrono::high_resolution_clock::now();
	auto transitionElapsedTime = (animationCurrentTime - transitionAnimationStartTime);
	float transitionAnimationTicks = transitionElapsedTime.count() / 1e9f;
	auto mouseElapsedTime = (animationCurrentTime - mouseAnimationStartTime);
	float mouseAnimationTicks = mouseElapsedTime.count() / 1e9f;

	currentMouseCard.data = player->sampleMousePick(frameIndex, 0);
	if (currentMouseCard.data > 64) {
		currentMouseCard.data = 0;
	}

	float transitionTargetDuration = 1.0f / params->animationSpeed;
	float mouseTargetDuration = 0.4f / params->animationSpeed;
	if (transitionAnimationTicks > transitionTargetDuration) {
		cards_2 = cards_3;
		params->transitionAnimationPlaying = false;
	}
	if (params->transitionAnimationPlaying) {
		float b = 4.5;
		float a = 0.5;
		float h = 0.1f;
		float x = transitionAnimationTicks / transitionTargetDuration;
		float xyzFactor = smoothInterpolate(x, a, b);
		float zLift = 0.0f;
		float modsFactor = 0.0f;
		if (x < 0.25f) {
			//lift
			zLift = smoothInterpolate(4 * x, a, b);
		}
		else if (0.25f <= x && x <= 0.75f) {
			zLift = 1.0f;
		}
		else {
			zLift = 1.0f - smoothInterpolate((4 * x) - 3, a, b);
		}
		if (x < 0.2f) {
			modsFactor = smoothInterpolate(5 * x, a, b);
		}
		else {
			modsFactor = 1.0f;
		}
		for (uint32_t i = 0; i < cards_1.size(); i++) {
			cards_2[i] = cards_1[i];
			cards_2[i].pos = (1.0f - xyzFactor) * cards_1[i].pos + xyzFactor * cards_3[i].pos;
			//only cards moving a large distance need to lift. 
			float sep = glm::length(cards_3[i].pos - cards_1[i].pos);
			if (sep > 0.1) {
				cards_2[i].pos.z += zLift * h;
			}
			cards_2[i].xy = (1.0f - xyzFactor) * cards_1[i].xy + xyzFactor * cards_3[i].xy;
			cards_2[i].xz = (1.0f - xyzFactor) * cards_1[i].xz + xyzFactor * cards_3[i].xz;
			cards_2[i].yz = (1.0f - xyzFactor) * cards_1[i].yz + xyzFactor * cards_3[i].yz;
			cards_2[i].mods = (1.0f - modsFactor) * cards_1[i].mods + modsFactor * cards_3[i].mods;
		}
	}
	else {
		if (mouseAnimationTicks > mouseTargetDuration && params->mouseAnimationPlaying) {
			cards_2 = cards_4;
			params->mouseAnimationPlaying = false;
		}
		if (params->mouseAnimationPlaying) {

			float b = 1.0;
			float a = 0.5;
			float h = 0.1f;
			float x = mouseAnimationTicks / mouseTargetDuration;
			float posFactor = smoothInterpolate(x, a, b);
			float xzFactor = 0.0f;
			constexpr float r = glm::pi<float>() / 20;
			float modsFactor = 0.0f;
			if (x < 0.25f) {
				//lift
				xzFactor = smoothInterpolate(4 * x, a, b);
			}
			else if (0.25f <= x && x <= 0.75f) {
				xzFactor = 1.0f;
			}
			else {
				xzFactor = 1.0f - smoothInterpolate((4 * x) - 3, a, b);
			}
			if (x < 0.2f) {
				modsFactor = smoothInterpolate(5 * x, a, b);
			}
			else {
				modsFactor = 1.0f;
			}
			for (uint32_t i = 0; i < cards_2.size(); i++) {
				cards_2[i] = cards_1[i];
				float sep = glm::length(cards_4[i].pos - cards_2[i].pos);
				cards_2[i].pos = (1.0f - posFactor) * cards_1[i].pos + posFactor * cards_2[i].pos;
				cards_2[i].xy = (1.0f - posFactor) * cards_1[i].xy + posFactor * cards_4[i].xy;
				cards_2[i].xz = (1.0f - posFactor) * cards_1[i].xz + posFactor * cards_4[i].xz;
				cards_2[i].yz = (1.0f - posFactor) * cards_1[i].yz + posFactor * cards_4[i].yz;
				cards_2[i].xz += (sep > 0.02f) ? xzFactor * r : 0.0f;
				cards_2[i].mods = (1.0f - modsFactor) * cards_1[i].mods + modsFactor * cards_4[i].mods;
			}

		}
		else {
			if (previousMouseCard.data != currentMouseCard.data) {
				//currentMouseCard.print();
				mouseAnimationStartTime = animationCurrentTime;
				cards_1 = cards_2;
				cards_4 = cards_3;
				cards_4[currentMouseCard.data].pos.y += 0.02f;
				cards_4[currentMouseCard.data].pos.z += 0.004f;
				params->mouseAnimationPlaying = true;
			}
		}
	}
	uint32_t drawCount = getCardMats();

	CardPushConstants pc{};
	pc.viewMat = viewMat;
	pc.viewPojectionMatrix = projMat * viewMat;
	pc.pos = glm::vec4(0.0f, 0.0f, -3.0f, 64.0f);
	pc.dir = glm::vec4(0);
	pc.colour = glm::vec4(0);
	pc.eyePos = glm::vec4(player->pos.x, player->pos.y, player->pos.z, 1.0f);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	memcpy(uniformsMapped[frameIndex], cardMats.data(), cardMats.size() * sizeof(cardMats[0]));

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CardPushConstants), &pc);
	vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 36, 0, 0);

	if (!params->mouseAnimationPlaying) {
		previousMouseCard = currentMouseCard;
	}
}
float CardRasterizer::smoothInterpolate(float x, float a, float b){
	return (glm::tanh(b * (x - a)) + glm::tanh(b * a)) / (glm::tanh(b * (1 - a)) + glm::tanh(b * a));
}
void CardRasterizer::getCardData() {
	CardData card{};
	card.mods = 0;
	durakGameState& state = durak->state;

	float height = 0.003f;

	//stock
	glm::vec2 stockPos = glm::vec2(0.8f, 0.5f);
	for (char i = 0; i < state.stock.size(); i++) {
		card.card = state.stock[i];
		card.pos = glm::vec3(stockPos, i * height);
		card.xy = 0;
		card.yz = 0;
		card.xz = glm::pi<float>();
		card.mods = 0.0f;
		if (i == 0) {
			card.xz = 0.0f;
			card.xy = glm::half_pi<float>(); 
			card.pos.x -= 0.02f;
		}
		cards_3[card.card.data] = card;
	}


	//local player's hand
	char locID = 0;
	if (locPlayerIndex != nullptr) {
		locID = *locPlayerIndex;
	}

	glm::vec2 locHandCent = glm::vec2(0.4f, -0.3f);
	float locHandRadius = 0.5f;
	constexpr float angleMax = 15.0f * glm::pi<float>() / 180.0f;
	float angleStep = 0.0f;
	float angleStart = 0.0f;
	char selectCard = 127;
	if (player->inputString.size() > 0) {
		try {
			selectCard = std::stoi(reinterpret_cast<char*>(player->inputString.data()));
		}
		catch (std::invalid_argument const& e) {
			selectCard = 126;
		}
	}

	if (state.hands[locID].size() == 1) {
		angleStart = 0.0f;
	}
	else if (state.hands[locID].size() == 2) {
		angleStart = -0.3f * angleMax;
		angleStep = 0.6f * angleMax;
	}
	else if (state.hands[locID].size() == 3) {
		angleStart = -0.5f * angleMax;
		angleStep = 0.5f * angleMax;
	}
	else if (state.hands[locID].size() == 4) {
		angleStart = -0.7f * angleMax;
		angleStep = 2.0f * 0.7f * angleMax / 3.0f;
	}
	else if (state.hands[locID].size() == 5) {
		angleStart = -0.85f * angleMax;
		angleStep = 2.0f * 0.85f * angleMax / 4.0f;
	}
	else {
		angleStart = -1.0f * angleMax;
		angleStep = 2.0f * angleMax / (state.hands[locID].size() - 1);
	}
	for (char i = 0; i < state.hands[locID].size(); i++) {
		float angle = angleStart + i * angleStep;
		float radius = locHandRadius * (1 + angle * 0.001f);
		card.pos = glm::vec3(glm::vec2(radius * glm::sin(angle), radius * glm::cos(angle)) + locHandCent, 0.0f);
		card.card = state.hands[locID][i];
		card.xy = -angle * 0.3f;
		card.yz = 0.0f;
		card.xz = 0.1f;
		card.mods = (i == selectCard) ? 1 : 0;
		cards_3[card.card.data] = card;
	}


	//opponent's hand
	char oppID = (locID + 1) % 2;
	glm::vec2 oppHandCent = glm::vec2(0.4f, 1.3f);
	float oppHandRadius = 0.5f;

	if (state.hands[oppID].size() == 1) {
		angleStart = 0.0f;
	}
	else if (state.hands[oppID].size() == 2) {
		angleStart = -0.3f * angleMax;
		angleStep = 0.6f * angleMax;
	}
	else if (state.hands[oppID].size() == 3) {
		angleStart = -0.5f * angleMax;
		angleStep = 0.5f * angleMax;
	}
	else if (state.hands[oppID].size() == 4) {
		angleStart = -0.7f * angleMax;
		angleStep = 2.0f * 0.7f * angleMax / 3.0f;
	}
	else if (state.hands[oppID].size() == 5) {
		angleStart = -0.85f * angleMax;
		angleStep = 2.0f * 0.85f * angleMax / 4.0f;
	}
	else {
		angleStart = -1.0f * angleMax;
		angleStep = 2.0f * angleMax / (state.hands[oppID].size() - 1);
	}

	for (char i = 0; i < state.hands[oppID].size(); i++) {
		float angle = angleStart + i * angleStep;
		float radius = oppHandRadius * (1 + angle * 0.001f);
		card.pos = glm::vec3(glm::vec2(radius * glm::sin(angle), -radius * glm::cos(angle)) + oppHandCent, 0.0f);
		card.card = state.hands[oppID][i];
		card.xy = -angle * 0.3f;
		card.yz = glm::pi<float>();
		card.xz = 0.1f;
		card.mods = 0.0f;
		cards_3[card.card.data] = card;
	}

	//discard;

	glm::vec2 discardPos = glm::vec2(0.9f, 0.5f);
	for (char i = 0; i < state.discard.size(); i++) {
		card.pos = glm::vec3(discardPos, height * i);
		card.card = state.discard[i];
		card.xy = 0.0f;
		card.xz = glm::pi<float>();
		card.yz = 0.0f;
		card.mods = 0.0f;
		cards_3[card.card.data] = card;
	}

	//table
	glm::vec2 tableStart = glm::vec2(0.2f, 0.5f);
	glm::vec2 tableStep = glm::vec2(0.09f, 0.0f);
	glm::vec2 tableCover = glm::vec2(0.0005f, -0.03f);
	char stacks = (state.table.size() + (state.table.size() % 2)) / 2;
	for (char i = 0; i < stacks; i++) {
		card.pos = glm::vec3(tableStart + float(i) * tableStep, 0.0f);
		card.card = state.table[2 * i];
		card.xy = 0.0f;
		card.xz = 0.0f;
		card.yz = 0.0f;
		card.mods = 0.0f;
		cards_3[card.card.data] = card;
		if ((2 * i) + 1 < state.table.size()) {
			card.pos = glm::vec3(tableStart + float(i) * tableStep + tableCover, height);
			card.card = state.table[2 * i + 1];
			card.xy = 0.0f;
			card.xz = 0.0f;
			card.yz = 0.0f;
			card.mods = 0.0f;
			cards_3[card.card.data] = card;
		}
	}
}
uint32_t CardRasterizer::getCardMats() {
	uint32_t index = 0;
	glm::vec3 cardDimensions = glm::vec3(1.0f, 95.0f / 71.0f, 0.015f);
	for (uint32_t i = 0; i < cards_2.size(); i++) {
		CardData card = cards_2[i];
		if (card.card.data == 0) {
			continue;
		}
		glm::mat4 sc = glm::mat4(cardSize * cardDimensions.x, 0.0f, 0.0f, 0.0f, 0.0f, cardSize * cardDimensions.y , 0.0f, 0.0f, 0.0f, 0.0f, cardSize * cardDimensions.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		glm::mat4 xy = glm::rotate(glm::mat4(1), card.xy, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 xz = glm::rotate(glm::mat4(1), card.xz, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 yz = glm::rotate(glm::mat4(1), card.yz, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 tr = glm::translate(glm::mat4(1), card.pos);

		//cardMats[index].mat = sc * xy * yz * xz * tr;
		cardMats[index].mat = tr * (yz * (xy * (xz * sc)));
		cardMats[index].card = card.card.data;
		cardMats[index].mod1 = card.mods;
		index++;
	}
	return index;
}

void CardRasterizer::cleanup() {
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	for (uint32_t i = 0; i < texImage.size(); i++) {
		vkDestroyImageView(device, texImageView[i], nullptr);
		vkDestroySampler(device, texSampler[i], nullptr);
		vkDestroyImage(device, texImage[i], nullptr);
	}

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

}