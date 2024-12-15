#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"

#include <fstream>
#include <charconv>
#include <glm/gtx/string_cast.hpp>

#include "cardRasterizer.h"
#include "structs.h"
#include "player.h"


void CardRasterizer::initCard_A(CardInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	renderPass = details.renderPass;

	msaaSamples = details.msaaSamples;

	memProperties = details.memProperties;
	shaderCode = details.shaderCode;

	player = details.player;

	table = details.gameTable.table;
	hands = details.gameTable.hands;
	wins = details.gameTable.wins;
	stock = details.gameTable.stock;

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

void CardRasterizer::initMemory(std::array<MemInit, 4> details) {
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
	maxSize = std::max(maxSize, uint32_t(sizeof(CardDataConstant) * CARD_COUNT));
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
	uniformBufferRegion = sizeof(CardDataConstant) * CARD_COUNT;
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

	vkGetBufferMemoryRequirements(device, vertexBuffer, &vertexRequirements.requirements);
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements.requirements);

	for (uint32_t i = 0; i < texImage.size(); i++) {
		vkGetImageMemoryRequirements(device, texImage[i], &texRequirements[i].requirements);
		texRequirements[i].flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

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
	samplerLayoutBinding.descriptorCount = texImage.size();
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

	std::array<VkShaderModule, 2> shaderModules;
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
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
	//rasterizer.cullMode = VK_CULL_MODE_NONE;
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;


	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
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
	constantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

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
	count->push_back(2 + texRequirements.size());
	details->push_back(vertexRequirements);
	details->push_back(uniformRequirements);
	for (uint32_t i = 0; i < texImage.size(); i++) {
		details->push_back(texRequirements[i]);
	}
}

void CardRasterizer::drawElements(VkCommandBuffer commandBuffer, uint32_t frameIndex, bool newCommand) {

	tableOffset = { 0.0f, 0.2f };
	stockOffset = { 0.002f, 0.0005f };
	cardHeightZero = 0.03f;
	cardHeightOffset = -0.002f;
	tablePositions = {//need to redefine to be more flexible
		glm::vec2(-4.2, 0.0f),
		glm::vec2(-3.0f, 0.0f),
		glm::vec2(-1.8f, 0.0f),
		glm::vec2(-0.6f, 0.0f),
		glm::vec2(0.6f, 0.0f),
		glm::vec2(1.8f, 0.0f),
		glm::vec2(3.0f, 0.0f),
		glm::vec2(4.2f, 0.0f) };
	handOffsets = {
		glm::vec2(-2.1f, 0.0f),
		glm::vec2(-0.7f, 0.0f),
		glm::vec2(0.7f, 0.0f),
		glm::vec2(2.1f, 0.0f) };
	handPositons = {
		glm::vec2(0.0f, -1.3f),
		glm::vec2(0.0f, 1.3f) };
	stockPosition = glm::vec2(4.4f, 0.0f);
	winPositions = {
		glm::vec2(-4.4f, -1.3f),
		glm::vec2(-4.4f, 1.3f) };


	glm::vec2 cardPos;
	glm::mat4 cardMat;
	float cardHeight;
	size_t cardIndex;
	float faceDirection;
	//newCommand = true;

	if (newCommand) {


		prevCardData = currentCardData;
		commandSubmitTime = std::chrono::high_resolution_clock::now();


		for (uint32_t i = 0; i < table->size(); i++) {
			for (uint32_t j = 0; j < (*table)[i].size(); j++) {
				cardPos = tablePositions[i] + tableOffset * float(j);
				cardHeight = cardHeightZero + j * cardHeightOffset;
				cardMat = {
					{1.0f, 0.0f, 0.0f, cardPos.x},
					{0.0f, 1.0f, 0.0f, cardPos.y},
					{0.0f, 0.0f, 1.0f, cardHeight},
					{0.0f, 0.0f, 0.0f, 1.0f} };
				cardIndex = (*table)[i][j].value();
				cardData[cardIndex].cardMat = glm::transpose(cardMat);
				currentCardData[cardIndex] = glm::vec4(cardPos.x, cardPos.y, cardHeight, 0.0f);

			}
		}
		for (uint32_t i = 0; i < stock->size(); i++) {
			cardHeight = cardHeightZero + cardHeightOffset * i;
			cardPos = stockPosition + float(i) * stockOffset;
			cardMat = {
				{1.0f, 0.0f, 0.0f, cardPos.x},
				{0.0f, -1.0f, 0.0f, cardPos.y},
				{0.0f, 0.0f, -1.0f, cardHeight},
				{0.0f, 0.0f, 0.0f, 1.0f} };
			cardIndex = (*stock)[i].value();
			cardData[cardIndex].cardMat = glm::transpose(cardMat);
			currentCardData[cardIndex] = glm::vec4(cardPos.x, cardPos.y, cardHeight, glm::pi<float>());
		}
		for (uint32_t i = 0; i < hands->size(); i++) {
			for (uint32_t j = 0; j < (*hands)[i]->size(); j++) {
				cardPos = handPositons[i] + handOffsets[j];
				cardHeight = cardHeightZero;
				faceDirection = (i == playerIndex) ? 1.0f : -1.0f;

				cardMat = {
					{1.0f, 0.0f, 0.0f, cardPos.x},
					{0.0f, faceDirection, 0.0f, cardPos.y},
					{0.0f, 0.0f, faceDirection, cardHeight},
					{0.0f, 0.0f, 0.0f, 1.0f} };
				cardIndex = (*(*hands)[i])[j].value();
				cardData[cardIndex].cardMat = glm::transpose(cardMat);
				currentCardData[cardIndex] = glm::vec4(cardPos.x, cardPos.y, cardHeight, 0.5*glm::pi<float>() + (0.5*-glm::pi<float>() * faceDirection));

			}
		}
		for (uint32_t i = 0; i < wins->size(); i++) {
			for (uint32_t j = 0; j < (*wins)[i]->size(); j++) {
				cardPos = winPositions[i] + winOffset * float(j);
				cardHeight = cardHeightZero + j * cardHeightOffset;
				cardMat = {
					{1.0f, 0.0f, 0.0f, cardPos.x},
					{0.0f, -1.0f, 0.0f, cardPos.y},
					{0.0f, 0.0f, -1.0f, cardHeight},
					{0.0f, 0.0f, 0.0f, 1.0f} };
				cardIndex = (*(*wins)[i])[j].value();
				cardData[cardIndex].cardMat = glm::transpose(cardMat);
				currentCardData[cardIndex] = glm::vec4(cardPos.x, cardPos.y, cardHeight, glm::pi<float>());

			}
		}
	}
	currentTime = std::chrono::high_resolution_clock::now();
	auto timeDuration = (std::chrono::duration<double>(currentTime - commandSubmitTime)).count();

	//std::cout << timeDuration << std::endl;

	double animationDuration = 1;
	double animationSmoothness = 5;


	float animationInterpolation = (timeDuration < animationDuration) ? 0.5f * glm::tanh(animationSmoothness * (timeDuration - (animationDuration / 2))) + 0.5f : 1.0f;
	glm::vec4 interpolatedCardData;
	glm::mat4 interpolatedMatData;
	
	for (uint32_t i = 0; i < currentCardData.size(); i++) {
		interpolatedCardData = (1.0f - animationInterpolation) * prevCardData[i] + animationInterpolation * currentCardData[i];
		interpolatedMatData = glm::mat4{
			{1.0f, 0.0, 0.0f, interpolatedCardData.x},
			{0.0f, glm::cos(interpolatedCardData.a), -glm::sin(interpolatedCardData.a), interpolatedCardData.y},
			{0.0f, glm::sin(interpolatedCardData.a), glm::cos(interpolatedCardData.a), interpolatedCardData.z},
			{0.0f, 0.0f, 0.0f, 1.0f} };
		cardData[i].cardMat = glm::transpose(interpolatedMatData);
		//std::cout << glm::to_string(cardData[i].cardMat) << std::endl;
	}
	





	std::vector<glm::vec4> positions;
	for (uint32_t i = 0; i < cardData.size(); i++) {
		positions.push_back(cardData[i].cardMat[3]);
		//std::cout << glm::to_string(cardData[i].cardMat) << std::endl;
		glm::vec4 testVec = { 1.0f, 1.0f, 1.0f, 1.0f };
		//std::cout << glm::to_string(cardData[i].cardMat * testVec) << std::endl;
	}
	//next step is instanced render of all cards.
	//and also how to tell fragment shader which texture to use?
	//probable easiest is a vertex attribute with texture index, so 0 is face, 1 is back, 2 is sides/no texture?
	//also need to set card values and render table
	//as well as view and projection matrices.
	//std::cout << 2 << std::endl;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	memcpy(uniformsMapped[frameIndex], cardData.data(), cardData.size() * sizeof(cardData[0]));

	vkCmdDraw(commandBuffer, vertices.size(), 52, 0, 0);
	frameCounter++;
	if (frameCounter == 120) {
		frameCounter = 0;
		cardCounter++;
	}


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