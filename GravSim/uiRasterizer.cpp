#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include <fstream>
#include <charconv>

#include "uiRasterizer.h"
#include "structs.h"
#include "player.h"

void UIRasterizer::initUI_A(UIInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	renderPass = details.renderPass;

	msaaSamples = details.msaaSamples;

	memProperties = details.memProperties;
	shaderCode = details.shaderCode;

	player = details.player;

	aspectRatio = details.aspectRatio;

	initFreetype();
	createBuffers();

}

void UIRasterizer::initUI_B() {
	createImageView();
	createSampler();
	createDescriptorSets();
	createPipeline();
}

void UIRasterizer::initMemory(std::array<MemInit, 3> details) {
	vertexMemory = details[0];
	uniformBufferMemory = details[1];
	texMemory = details[2];

	vkBindBufferMemory(device, vertexBuffer, vertexMemory.memory, vertexMemory.offset);
	vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory.memory, uniformBufferMemory.offset);
	vkBindImageMemory(device, texImage, texMemory.memory, texMemory.offset);

	void* data;
	vkMapMemory(device, uniformBufferMemory.memory, uniformBufferMemory.offset, uniformBufferMemory.range, 0, &data);

	uniformBufferMapped = static_cast<char*>(data);

	for (size_t i = 0; i < uniformsMapped.size(); i++) {
		uniformsMapped[i] = uniformBufferMapped + i * uniformBufferRegion;
	}

}

void UIRasterizer::initBufferData_A(MemoryDetails* stagingRequirements) {
	uint32_t maxSize = std::max(uint32_t(sizeof(texPixels[0]) * texPixels.size()), uniformBufferSize);
	maxSize = std::max(maxSize, uint32_t(sizeof(vertices[0]) * vertices.size()));
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = maxSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &createInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI staging Buffer"); }

	vkGetBufferMemoryRequirements(device, stagingBuffer, &(stagingRequirements->requirements));

	stagingRequirements->flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}
void UIRasterizer::initBufferData_B(VkCommandBuffer transferCommandBuffer, VkQueue transferQueue, MemInit memory){
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

	copySize = texPixels.size() * sizeof(texPixels[0]);
	memcpy(data, texPixels.data(), copySize);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texImage;
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
	region.imageExtent = { texWidth, texHeight, 1 };
	
	vkCmdCopyBufferToImage(transferCommandBuffer, stagingBuffer, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

	vkDestroyFence(device, transferFence, nullptr);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void UIRasterizer::createBuffers() {
	//create texture atlas

	VkDeviceSize imageSize = texWidth * texHeight * sizeof(texPixels[0]);
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(texWidth);
	imageInfo.extent.height = static_cast<uint32_t>(texHeight);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.format = VK_FORMAT_R8_UINT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	if (vkCreateImage(device, &imageInfo, nullptr, &texImage) != VK_SUCCESS) { throw std::runtime_error("Failed to create texture atlas image"); }

	uniformBufferRegion = sizeof(char) * stringLength;
	uniformBufferSize = uniformBufferRegion * FRAMES_IN_FLIGHT;

	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = sizeof(vertices[0]) * vertices.size();
	if (vkCreateBuffer(device, &createInfo, nullptr, &vertexBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI vertex buffer"); }
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo.size = uniformBufferSize;
	if (vkCreateBuffer(device, &createInfo, nullptr, &uniformBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI uniform buffer"); }

	vkGetBufferMemoryRequirements(device, vertexBuffer, &vertexRequirements.requirements);
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements.requirements);

	vkGetImageMemoryRequirements(device, texImage, &texRequirements.requirements);

	vertexRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	texRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uniformRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


}
void UIRasterizer::createImageView() {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8_UINT;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	
	if (vkCreateImageView(device, &viewInfo, nullptr, &texImageView) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI image view"); }

}
void UIRasterizer::createSampler() {
	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_NEAREST;
	createInfo.minFilter = VK_FILTER_NEAREST;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.anisotropyEnable = VK_FALSE;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable =  VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &createInfo, nullptr, &texSampler) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI sampler"); }
}

void UIRasterizer::createDescriptorSets() {
	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 0;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI descriptor set layout"); }

	std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { descriptorSetLayout, descriptorSetLayout, descriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate UI descriptor sets"); }



	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = i * uniformBufferRegion;
		bufferInfo.range = uniformBufferRegion;

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texImageView;
		imageInfo.sampler = texSampler;

		VkWriteDescriptorSet bufferWrite{};
		bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		bufferWrite.dstSet = descriptorSets[i];
		bufferWrite.dstBinding = 0;
		bufferWrite.dstArrayElement = 0;
		bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferWrite.descriptorCount = 1;
		bufferWrite.pBufferInfo = &bufferInfo;

		VkWriteDescriptorSet imageWrite{};
		imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageWrite.dstSet = descriptorSets[i];
		imageWrite.dstBinding = 1;
		imageWrite.dstArrayElement = 0;
		imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrite.descriptorCount = 1;
		imageWrite.pImageInfo = &imageInfo;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = { bufferWrite, imageWrite };

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void UIRasterizer::createPipeline() {

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
	attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
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
	constantRange.size = sizeof(UIPushConstants);
	constantRange.offset = 0;
	constantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &constantRange;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI pipeline layout"); }
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

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create UI pipeline"); }

	for (uint32_t i = 0; i < shaderModules.size(); i++) {
		vkDestroyShaderModule(device, shaderModules[i], nullptr);
	}
}

void UIRasterizer::getMemoryRequirements(std::vector<MemoryDetails>* details, std::vector<uint16_t>* count) {
	count->push_back(3);
	details->push_back(vertexRequirements);
	details->push_back(uniformRequirements);
	details->push_back(texRequirements);
}

void UIRasterizer::initFreetype() {
	if (FT_Init_FreeType(&library)) { throw std::runtime_error("Failed to intialize FreeType Library"); }

	if (FT_New_Face(library, "C:/Windows/Fonts/CascadiaCode.ttf", 0, &face)) { throw std::runtime_error("Failed to load Font"); }

	FT_Set_Pixel_Sizes(face, 0, 96);


	char character = 22;

	uint32_t glyphIndex = FT_Get_Char_Index(face, 'c');

	FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

	std::vector<FT_GlyphSlot> glyphs;

	int16_t xMin = 10;
	int16_t xMax = 10;
	int16_t bxMax = 0;
	int16_t yMin = 10;
	int16_t yMax = 10;
	int16_t advMax = 10;

	std::vector<textBitmapWrapper> textContainers;

	textContainers.reserve(charCount);

	for (int16_t i = charStart; i < charStart + charCount; i++) {
		//std::cout << i << ": ";
		FT_Load_Char(face, i, FT_LOAD_RENDER);
		//std::cout << face->glyph->bitmap.width << ", " << face->glyph->bitmap_top << ", " << face->glyph->advance.x / 64 << std::endl;

		textBitmapWrapper tempContainer;
		tempContainer.character = i;
		tempContainer.advance = face->glyph->advance.x / 64;
		tempContainer.bearingX = face->glyph->bitmap_left;
		tempContainer.bearingY = face->glyph->bitmap_top;
		tempContainer.address = new FT_Bitmap;

		xMin = std::min(xMin, tempContainer.bearingX);
		xMax = std::max(xMax, static_cast<int16_t>(face->glyph->bitmap.width));
		bxMax = std::max(bxMax, tempContainer.bearingX);
		yMin = std::min(yMin, (int16_t)(tempContainer.bearingY - face->glyph->bitmap.rows));
		yMax = std::max(yMax, tempContainer.bearingY);
		advMax = std::max(advMax, tempContainer.advance);


		FT_Bitmap_Init(tempContainer.address);
		FT_Bitmap_Copy(library, &(face->glyph->bitmap), tempContainer.address);

		textContainers.push_back(tempContainer);
	}
	
	charWidth = 4*ceil(((float)(xMax + bxMax - xMin))/4);
	texWidth = charCount * charWidth;
	texHeight = 4 * ceil(((float)(yMax - yMin))/4);
	charAdvance = advMax;
	rawCharDimensions.y = rawCharDimensions.x * float(texHeight) / float(charWidth);
	int16_t orgHeight = -yMin;
	int16_t orgWidth = xMin;

	
	texPixels.resize(texWidth * texHeight);
	int16_t orgX = 0;
	int16_t orgY = 0;
	int16_t rows = 0;
	int16_t cols = 0;
	int16_t penX = 0;
	int16_t penY = 0;



	for (int16_t i = 0; i < charCount; i++) {
		rows = textContainers[i].address->rows;
		cols = textContainers[i].address->width;
		orgX = i * charWidth + orgWidth;
		orgY = orgHeight;
		penX = orgX + textContainers[i].bearingX;
		penY = texHeight - orgY - textContainers[i].bearingY;


		for (int32_t x = 0; x < cols; x++) {
			for (int32_t y = 0; y < rows; y++) {
				int32_t index = (penY + y) * texWidth + penX + x;
				int16_t index2 = y * (textContainers[i].address->pitch) + x;
				auto value = textContainers[i].address->buffer[y * (textContainers[i].address->pitch) + x];
				texPixels[index] = value;
				
			}
		}
		FT_Bitmap_Done(library, textContainers[i].address);
		delete textContainers[i].address;
	}

	vertices = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(0, 1), glm::vec2(1, 0), glm::vec2(1, 1) };


	FT_Done_FreeType(library);

	std::ofstream file;
	file.open("out.csv");

	

	for (int32_t y = 0; y < texHeight; y++) {
		for (int32_t x = 0; x < texWidth; x++) {
			file << uint32_t(texPixels[(y * texWidth) + x]) <<", ";
		}
		file << "\n";
	}

	file.close();
}

void UIRasterizer::drawElements(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
	UIPushConstants pushConstant{};
	pushConstant.texAdvance = float(charAdvance) / texWidth;
	pushConstant.charDimensions = glm::vec2(float(charWidth) / float(texWidth), 1);
	pushConstant.renderStage = 1;

	glm::vec2 screenPos1 = glm::vec2(-0.5, -0.5);
	glm::vec2 screenDim1 = glm::vec2(10, 0.25);

	pushConstant.screenPosition = screenPos1;
	pushConstant.texDimensions = screenDim1;



	//memcpy(uniformsMapped[frameIndex], str.data(), sizeof(str[0]) * str.size());

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstants), &pushConstant);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	charVec.clear();

	std::vector<char> charData;
	//std::vector<uint32_t> blockLengths;
	//std::vector<uint32_t> blockCounts;
	//std::vector<uint32_t> blockConfigs;
	//std::vector<glm::vec2> textPositions;
	//std::vector<glm::vec2> textDimensions;
	glm::vec2 blockCursor = glm::vec2(-1, -1);

	std::vector<uint32_t> hBlockCountc;
	std::vector<uint32_t> hBlockCount;
	std::vector<uint32_t> nBlockCounth;
	std::vector<uint32_t> nBlockCount;
	std::vector<uint32_t> vBlockCountn;
	std::vector<uint32_t> vBlockCount;

	UIBox* workingBox;
	UIText* workingText;
	uint32_t localCharCount = 0;
	std::vector<char> localChar = { 'a','9' };

	UIText sText{};
	sText.charCount = 2;
	sText.dataP = &localChar;
	sText.config = UI_DATA_CHAR_VEC;
	
	charDimensions.y = rawCharDimensions.y / *aspectRatio;
	charDimensions.x = charDimensions.x;

	
	UIText tTexts[6] = { sText, sText,sText,sText,sText,sText };
	tTexts[0].config |= UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE;
	tTexts[1].config |= UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE;
	tTexts[2].config |= UI_ALIGNMENT_H_C | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE;
	tTexts[3].config |= UI_ALIGNMENT_H_C | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE;
	tTexts[4].config |= UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_B | UI_NEWLINE_FALSE;
	tTexts[5].config |= UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_B | UI_NEWLINE_TRUE;

	UIBox tBox;
	tBox.textCount = 6;
	tBox.dataP = &tTexts[0];

	//player->boxes.resize(1);
	//player->boxes.push_back(tBox);

	float px;
	float py;
	float dx;
	float dy;
	uint32_t workingConfig = 0;
	float texAdvance = float(charAdvance) / float(charWidth) * charDimensions.x;

	glm::vec2 screenCoords{};
	glm::vec2 boxScreenCoords = glm::vec2(0.1,0.1);
	glm::vec2 boxDimCoords = glm::vec2(0.9,0.9);
	glm::vec2 blockBoxCoords = glm::vec2(0,0);
	glm::vec2 blockScreenCoords{};

	std::vector<glm::vec2> localPos;

	UIPushConstants pc{};

	std::vector<uint32_t> boxCountc;
	boxCountc.push_back(0);

	for (uint32_t i = 0; i < player->boxes.size(); i++) {
		//blockCounts.clear();
		//blockConfigs.clear();
		//blockLengths.clear();
		//blockConfigs.push_back(0);
		//workingBox = (player->boxes)[i];
		//for (uint32_t j = 0; j < workingBox.textCount; j++) {
		//	workingText = workingBox.dataP + j;
		//	populateCharVector(workingText, &charCount);
		//	if ((blockConfigs[blockConfigs.size() - 1] & UI_NEWLINE_MASK) == UI_NEWLINE_TRUE) {
		//		blockConfigs.push_back(workingText->config);
		//		blockCounts.push_back(charCount);
		//		blockLengths.push_back(1);
		//	}
		//	else if ((blockConfigs[blockConfigs.size() - 1] & UI_ALIGNMENT_H_MASK) != (workingText->config & UI_ALIGNMENT_H_MASK)) {
		//		blockConfigs.push_back(workingText->config);
		//		blockCounts.push_back(charCount);
		//		blockLengths.push_back(1);
		//	}
		//	else if ((blockConfigs[blockConfigs.size() - 1] & UI_ALIGNMENT_V_MASK) != (workingText->config & UI_ALIGNMENT_V_MASK)) {
		//		blockConfigs.push_back(workingText->config);
		//		blockConfigs.push_back(charCount);
		//		blockLengths.push_back(1);
		//	}
		//	else {
		//		blockCounts[blockCounts.size() - 1] += charCount;
		//		blockLengths[blockLengths.size() - 1] += 1;
		//		blockConfigs[blockConfigs.size() - 1] = workingText->config;
		//	}
		//}
		//uint32_t blockIndex = 0;
		//uint32_t newlineCount = 0;
		//for (uint32_t j = 0; j < blockConfigs.size(); j++) {
		//	for (uint32_t k = 0; k < blockLengths[j]; k++) {
		//		switch (blockConfigs[j] & UI_ALIGNMENT_H_MASK) {
		//		case UI_ALIGNMENT_H_L:
		//		}
		//	}
		//}
		vBlockCount.clear();
		nBlockCount.clear();
		hBlockCount.clear();
		hBlockCountc.clear();
		nBlockCounth.clear();
		vBlockCountn.clear();


		

		vBlockCount.push_back(0);
		nBlockCount.push_back(0);
		hBlockCount.push_back(0);



		workingBox = &player->boxes[i];
		uint32_t prevConfig = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE;

		if (workingBox->textCount == 0) {
			continue;
		}

		for (uint32_t j = 0; j < workingBox->textCount; j++) {
			workingText = workingBox->dataP + j;

			if ((workingText->config & UI_ALIGNMENT_V_MASK) != (prevConfig & UI_ALIGNMENT_V_MASK)) {
				//action if v_block different
				vBlockCount.push_back(1);
				nBlockCount.push_back(1);
				hBlockCount.push_back(1);
				prevConfig = (workingText->config & UI_ALIGNMENT_V_MASK) | UI_ALIGNMENT_H_L | UI_NEWLINE_FALSE;
			}
			else if ((workingText->config & UI_NEWLINE_MASK) == UI_NEWLINE_TRUE) {
				//action if newline
				vBlockCount[vBlockCount.size() - 1]++;
				nBlockCount.push_back(1);
				hBlockCount.push_back(1);
				prevConfig = (workingText->config & UI_ALIGNMENT_V_MASK) | UI_ALIGNMENT_H_L | UI_NEWLINE_FALSE;
			}
			else if ((workingText->config & UI_ALIGNMENT_H_MASK) != (prevConfig & UI_ALIGNMENT_H_MASK)) {
				//action if h_block different
				vBlockCount[vBlockCount.size() - 1]++;
				nBlockCount[nBlockCount.size() - 1]++;
				hBlockCount.push_back(1);
				prevConfig = workingText->config;
			}
			else {
				hBlockCount[hBlockCount.size() - 1]++;
				nBlockCount[nBlockCount.size() - 1]++;
				vBlockCount[vBlockCount.size() - 1]++;
			}
		}

		uint32_t hBlockIndex = 0;
		uint32_t hBlockLimit = hBlockCount[0];
		uint32_t nBlockIndex = 0;
		uint32_t nBlockLimit = nBlockCount[0];
		uint32_t vBlockIndex = 0;
		uint32_t vBlockLimit = vBlockCount[0];
		uint32_t texIndex = 0;
		uint32_t lineIndex = 0;
		uint32_t charIndex = 0;


		nBlockCounth.push_back(0);
		vBlockCountn.push_back(0);
		hBlockCountc.push_back(0);

		for (uint32_t j = 0; j < workingBox->textCount; j++) {
			workingText = workingBox->dataP + j;
			if (j >= vBlockLimit) {
				nBlockIndex++;
				nBlockLimit += nBlockCount[hBlockIndex];
				hBlockIndex++;
				hBlockLimit += hBlockCount[hBlockIndex];
				vBlockIndex++;
				vBlockLimit += vBlockCount[vBlockIndex];
				nBlockCounth[nBlockCounth.size() - 1]++;
				vBlockCountn[vBlockCountn.size() - 1]++;
				hBlockCountc.push_back(0);
				nBlockCounth.push_back(0);
				vBlockCountn.push_back(0);
				
			}
			else if (j >= nBlockLimit) {
				nBlockIndex++;
				nBlockLimit += nBlockCount[nBlockIndex];
				hBlockIndex++;
				hBlockLimit += hBlockCount[hBlockIndex];
				nBlockCounth[nBlockCounth.size() - 1]++;
				vBlockCountn[vBlockCountn.size() - 1]++;
				hBlockCountc.push_back(0);
				nBlockCounth.push_back(0);
			}
			else if (j >= hBlockLimit) {
				hBlockIndex++;
				hBlockLimit += hBlockCount[hBlockIndex];
				nBlockCounth[nBlockCounth.size() - 1]++;;
				hBlockCountc.push_back(0);
			}
			populateCharVector(workingText, &localCharCount);
			hBlockCountc[hBlockCountc.size() - 1] += localCharCount;
		}
		nBlockCounth[nBlockCounth.size() - 1]++;
		vBlockCountn[vBlockCountn.size() - 1]++;

		hBlockIndex = 0;
		hBlockLimit = 0;
		nBlockIndex = 0;
		nBlockLimit = nBlockCount[0];
		vBlockIndex = 0;
		vBlockLimit = vBlockCount[0];
		


		for (uint32_t k = 0; k < hBlockCount.size(); k++){
			workingConfig = (workingBox->dataP + texIndex)->config;
			workingText = workingBox->dataP + texIndex;
			switch (workingConfig & UI_ALIGNMENT_H_MASK) {
			case UI_ALIGNMENT_H_L:
				px = 0;
				dx = 0;
				break;
			case UI_ALIGNMENT_H_C:
				px = 0.5;
				dx = hBlockCountc[k] / 2;
				break;
			case UI_ALIGNMENT_H_R:
				px = 1;
				dx = hBlockCountc[k];
				break;
			default:
				throw std::runtime_error("UI_ALIGNENT_H fall through");
			}
			switch (workingConfig & UI_ALIGNMENT_V_MASK) {
			case UI_ALIGNMENT_V_T:
				py = 0;
				dy = 0.0f - lineIndex;
				break;
			case UI_ALIGNMENT_V_C:
				py = 0.5;
				dy = vBlockCountn[vBlockIndex] / 2 - lineIndex;
				break;
			case UI_ALIGNMENT_V_B:
				py = 1;
				dy = vBlockCountn[vBlockIndex] - lineIndex;
				break;
			default:
				throw std::runtime_error("UI_ALIGNENT_V fall through");
			}
			
			blockBoxCoords = glm::vec2(px - (texAdvance * dx), py - (charDimensions.y * dy));
			blockScreenCoords = glm::vec2(workingBox->pos.x + (blockBoxCoords.x * workingBox->size.x), workingBox->pos.y + (blockBoxCoords.y * workingBox->size.y));
			pc.charDimensions = charDimensions;
			pc.screenPosition = blockScreenCoords;
			pc.renderStage = 3;
			pc.texAdvance = texAdvance;
			pc.texDimensions = glm::vec2(1.0f / charCount, 1);
			pc.instanceOffset = charIndex + boxCountc[i];
			pc.inColour = glm::vec4(workingText->colour.x, workingText->colour.y, workingText->colour.z, 0.0f);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstants), &pc);
			vkCmdDraw(commandBuffer, vertices.size(), hBlockCountc[k], 0, 0);
			charIndex += hBlockCountc[k];

			localPos.push_back(blockScreenCoords);

			texIndex += hBlockCount[k];
			if (texIndex >= nBlockLimit && texIndex < workingBox->textCount) {
				nBlockIndex++;
				nBlockLimit += nBlockCount[nBlockIndex];
				lineIndex++;
			}
			//redo line index, counting hblocks per line, need to count newline blocks per vblock
			if (texIndex >= vBlockLimit && texIndex < workingBox->textCount) {
				vBlockIndex++;
				vBlockLimit += vBlockCount[vBlockIndex];
				lineIndex = 0;
			}

		}
		boxCountc.push_back(charVec.size());

	}
	memcpy(uniformsMapped[frameIndex], charVec.data(), charVec.size() * sizeof(charVec[0]));



	//vkCmdDraw(commandBuffer, vertices.size(), 11, 0, 0);

	//std::vector<glm::uvec4> charVecData(charData.size()/16);
	//memcpy(charVecData.data(), charData.data(), charData.size() * sizeof(char));
	//for (uint32_t i = 0; i < charCounter; i++) {
	//	uint32_t uboVecIndex = i >> 4;
	//	uint32_t uboValIndex = (i >> 2) & 3;
	//	uint32_t uboShiftIndex = i & 3;
	//	uint32_t uboShiftValue = 8 * uboShiftIndex;
	//	uint32_t uboMask = 127;

	//	glm::uvec4 uboVec = charVecData[uboVecIndex];
	//	uint32_t uboVal = uboVec[uboValIndex];
	//	uint32_t char1 = (uboVal >> uboShiftValue) & uboMask;
	//	char char2 = char1;
	//	std::cout << char2;
	//}





	
}

void UIRasterizer::populateCharVector(UIText* text,uint32_t* charCounts) {
	uint32_t tConfig = text->config & UI_DATA_MASK;
	uint32_t endLimit = 0;


	switch (tConfig) {
	case UI_DATA_UINT32_T:
		charVec.resize(10 + charVec.size());
		std::to_chars(charVec.data() + charVec.size() - 10, charVec.data() + charVec.size(), *reinterpret_cast<uint32_t*>(text->dataP));
		for (uint32_t i = 0; i < 10; i++) {
			if (charVec[charVec.size() - 1 - i] != 0) {
				endLimit = i;
				break;
			}
		}
		charVec.resize(charVec.size() - endLimit);
		*charCounts = 10 - endLimit;

		break;
	case UI_DATA_FLOAT:
		charVec.push_back('0');
		*charCounts = 1;
		break;
	case UI_DATA_CHAR_VEC:
		charVec.resize(charVec.size() + reinterpret_cast<std::vector<char>*>(text->dataP)->size());
		std::memcpy(charVec.data() + charVec.size() - reinterpret_cast<std::vector<char>*>(text->dataP)->size(), reinterpret_cast<std::vector<char>*>(text->dataP)->data(), reinterpret_cast<std::vector<char>*>(text->dataP)->size());
		*charCounts = reinterpret_cast<std::vector<char>*>(text->dataP)->size();
		break;
	case UI_DATA_STRING:
		for (uint32_t i = 0; i < reinterpret_cast<std::string*>(text->dataP)->size(); i++) {
			charVec.push_back((*reinterpret_cast<std::string*>(text->dataP))[i]);
		}
		*charCounts = reinterpret_cast<std::string*>(text->dataP)->size();
		break;
	default:
		throw std::runtime_error("Unsupported text data type");
		break;
	}
}

void UIRasterizer::cleanup() {
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkDestroyImageView(device, texImageView, nullptr);
	vkDestroySampler(device, texSampler, nullptr);
	vkDestroyImage(device, texImage, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

}