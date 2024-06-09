#include "baseRasterizer.h"

#include <cmath>

void BaseRasterizer::initRast(RastInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	renderPass = details.renderPass;

	shaderCode = details.shaderCode;

	meshes = details.meshes;
	particleCount = details.particleCount;

	memProperties = details.memProperties;

	gravStorageBuffer = details.gravStorageBuffer;

	createPipeline();
}

void BaseRasterizer::createPipeline() {

	//first create shader modules
	std::array<VkShaderModule, 2> shaderModules;
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	std::array<VkShaderStageFlagBits, 2> flagBits{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };


	for (uint32_t i = 0; i < shaderCode.size(); i++) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode[i]->size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode[i]->data());

		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModules[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizerShaderModule"); }

		shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[i].stage = flagBits[i];
		shaderStages[i].module = shaderModules[i];
		shaderStages[i].pName = "main";
	}

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	//rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

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

	VkPushConstantRange camera{};
	camera.offset = 0;
	camera.size = sizeof(CameraPushConstants);
	camera.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &camera;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizer pipelineLayout"); }

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

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizer pipeline"); }

	for (uint32_t i = 0; i < shaderModules.size(); i++) {
		vkDestroyShaderModule(device, shaderModules[i], nullptr);
	}
}

void BaseRasterizer::createDescriptorSets() {
	//start by creating the descriptor set layout
	VkDescriptorSetLayoutBinding ubo{};
	ubo.binding = 0;
	ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo.descriptorCount = 1;
	ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding storage{};
	storage.binding = 1;
	storage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storage.descriptorCount = 1;
	storage.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	storage.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo, storage };

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizerDescriptorSetLayout"); }
	
	//now allocate the descriptor sets

	std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { descriptorSetLayout, descriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate BaseRasterizer descriptor sets"); }

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = i * sizeof(UniformBufferObject);
		bufferInfo.range = sizeof(UniformBufferObject);



		VkDescriptorBufferInfo ssboInfo{};
		ssboInfo.buffer = gravStorageBuffer;
		ssboInfo.offset = storageSize * ((i + 2) % FRAMES_IN_FLIGHT);
		ssboInfo.range = storageSize;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &ssboInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void BaseRasterizer::createBuffers() {
	//first make vertex and index buffers
	vertexOffsets.resize(meshes.size());
	indexOffsets.resize(meshes.size());
	vertexBuffers.resize(meshes.size());
	indexBuffers.resize(meshes.size());

	VkBufferCreateInfo vertexInfo{};
	vertexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo indexInfo{};
	indexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//make 1 for every mesh
	for (size_t i = 0; i < meshes.size(); i++) {

		vertexInfo.size = meshes[i].vertexCount * sizeof(Vertex);
		indexInfo.size = meshes[i].indexCount * sizeof(uint16_t);

		maxBufferSize = std::max(std::max(vertexInfo.size, indexInfo.size), maxBufferSize);
		


		if (vkCreateBuffer(device, &vertexInfo, nullptr, &vertexBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizer vertexBuffer"); }
		if (vkCreateBuffer(device, &indexInfo, nullptr, &indexBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizer indexBuffer"); }

		VkMemoryRequirements vertexMem;
		VkMemoryRequirements indexMem;

		vkGetBufferMemoryRequirements(device, vertexBuffers[i], &vertexMem);
		vkGetBufferMemoryRequirements(device, indexBuffers[i], &indexMem);

		if (i == 0) {
			vertexRequirements = vertexMem;
			indexRequirements = indexMem;
			vertexOffsets[i] = 0;
			indexOffsets[i] = 0;
		}
		else {
			vertexOffsets[i] = vertexRequirements.size;
			indexOffsets[i] = indexRequirements.size;

			vertexRequirements.size += vertexMem.size;
			indexRequirements.size += indexMem.size;
		}
	}
	vertexOffsets.push_back(vertexRequirements.size);
	indexOffsets.push_back(indexRequirements.size);

	//now make uniform buffer
	VkDeviceSize bufferSize = sizeof(UniformBufferObject) * FRAMES_IN_FLIGHT;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create BaseRasterizer uniform buffers"); }

	
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements);



}

void BaseRasterizer::initMemory(std::array<MemInit, 3> details) {
	vertexMemory = details[0];
	indexMemory = details[1];
	uniformBufferMemory = details[2];

	for (size_t i = 0; i < meshes.size(); i++) {
		vkBindBufferMemory(device, vertexBuffers[i], vertexMemory.memory, vertexMemory.offset + vertexOffsets[i]);
		vkBindBufferMemory(device, indexBuffers[i], indexMemory.memory, indexMemory.offset + indexOffsets[i]);
	}
}

void BaseRasterizer::initBufferData_A(VkMemoryRequirements* stagingRequirements) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = maxBufferSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &createInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create baseRasterizer staging buffer"); }

	vkGetBufferMemoryRequirements(device, stagingBuffer, stagingRequirements);

	(*stagingRequirements).memoryTypeBits |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}
void BaseRasterizer::initBufferData_B(VkCommandBuffer transferCommandBuffer, VkQueue transferQueue, MemInit memory) {
	stagingMemory = memory;
	
	vkBindBufferMemory(device, stagingBuffer, stagingMemory.memory, stagingMemory.offset);

	//create neccessary sync objects
	VkFence transferFence;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(device, &fenceInfo, nullptr, &transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to create transfer fence for GravDataSync"); }

	//record copy operation
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	
	void* data;
	vkMapMemory(device, stagingMemory.memory, stagingMemory.offset, stagingMemory.range, 0, &data);

	for (size_t i = 0; i < meshes.size(); i++) {
		//vertices
		//step 1 copy data to to buffer
		memcpy(data, meshes[i].vertices->data(), meshes[i].vertices->size());

		vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
		VkBufferCopy copy{};
		copy.size = meshes[i].vertices->size();
		copy.srcOffset = 0;
		copy.dstOffset = 0;
		//step 2 perform copy
		vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, vertexBuffers[i], 1, &copy);
		vkEndCommandBuffer(transferCommandBuffer);
		vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
		vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
		vkResetCommandBuffer(transferCommandBuffer, 0);
	}
	for (size_t i = 0; i < meshes.size(); i++) {
		//indices
		//step 1 copy data to to buffer
		memcpy(data, meshes[i].indices->data(), meshes[i].indices->size());
		
		vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
		VkBufferCopy copy{};
		copy.size = meshes[i].indices->size();
		copy.srcOffset = 0;
		copy.dstOffset = 0;
		//step 2 perform copy
		vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, vertexBuffers[i], 1, &copy);
		vkEndCommandBuffer(transferCommandBuffer);
		vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
		vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
		vkResetCommandBuffer(transferCommandBuffer, 0);
	}

	vkUnmapMemory(device, stagingMemory.memory);

	stagingMemory = {};


	//cleanup resources



	vkDestroyFence(device, transferFence, nullptr);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	
}
void BaseRasterizer::drawObjects(VkCommandBuffer commandBuffer, uint32_t frameIndex, CameraPushConstants* camera) {
	

	//render pass is already going so start by binding pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CameraPushConstants), camera);

	//then for each model
	VkDeviceSize offsets[] = { 0 };

	for (size_t i = 0; i < meshes.size(); i++) {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[i], offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffers[i], 0, VK_INDEX_TYPE_UINT16);
		//for single model
		vkCmdDrawIndexed(commandBuffer, meshes[i].indexCount, 1, 0, 0, 0);
		//for instanced model everywhere
		vkCmdDrawIndexed(commandBuffer, meshes[i].indexCount, particleCount, 0, 0, 0);
	}
}	

void BaseRasterizer::getMemoryRequirements(std::vector<VkMemoryRequirements>* mem, std::vector<uint16_t>* count) {
	mem->push_back(vertexRequirements);
	mem->push_back(indexRequirements);
	mem->push_back(uniformRequirements);
	count->push_back(3);
}

void BaseRasterizer::cleanup() {
	for (size_t i = 0; i < meshes.size(); i++) {
		vkDestroyBuffer(device, vertexBuffers[i], nullptr);
		vkDestroyBuffer(device, indexBuffers[i], nullptr);
	}
	vkUnmapMemory(device, uniformBufferMemory.memory);
	vkDestroyBuffer(device, uniformBuffer, nullptr);

	vkFreeDescriptorSets(device, descriptorPool, descriptorSets.size(), descriptorSets.data());


	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}