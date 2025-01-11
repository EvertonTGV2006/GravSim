#include "particleRasterizer.h"

#include <cmath>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>

void particleRasterizer::initRast_A(RastInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	renderPass = details.renderPass;

	msaaSamples = details.msaaSamples;

	memcpy(shaderCode.data(), details.shaderCode.data(), shaderCode.size() * sizeof(shaderCode[0]));

	meshes = details.meshes;


	memProperties = details.memProperties;

	planets = details.planets;

	createBuffers();

	(*planets)[0].pos_0 = glm::vec3(0.0f, 0.0f, 0.0f);
	(*planets)[0].radius = 6378e3f;
	(*planets)[0].vel_0 = glm::vec3(0.0f, 0.0f, 0.0f);
	(*planets)[0].mass = 5.9722e24f;
	(*planets)[0].axis = glm::vec3(0.0f, glm::asin(glm::radians(23.5f)), glm::acos(glm::radians(23.5f)));
	(*planets)[0].theta = 0.0f;

	(*planets)[1].pos_0 = glm::vec3(0.4055e9/*10.0f*/, 0.0f, 0.0f);
	(*planets)[1].radius = 1738e3f/*0.5f*/;
	(*planets)[1].vel_0 = glm::vec3(0.0f, 0.970e3/*0.5f*/, 0.0f);
	(*planets)[1].mass = 0.07346e24f;
	(*planets)[1].axis = glm::vec3(0.0f, 0.0f, 1.0f);
	(*planets)[1].theta = 0.0f;

	for (uint32_t i = 2; i < planets->size(); i++) {
		(*planets)[i].pos_0 = glm::vec3(glm::linearRand<float>(0, 10) * 7e7, glm::linearRand<float>(0, 10) * 8e7, glm::linearRand<float>(0, 10) * 1e1);
		(*planets)[i].radius = glm::linearRand<float>(0.1f, 2.0f)*1e6f;
		//(*planets)[i].vel = glm::vec3(glm::linearRand<float>(0, 1)*1e3, glm::linearRand<float>(0, 1)*1e3, glm::linearRand<float>(0, 1)*1e0 );
		(*planets)[i].vel_0 = glm::sqrt(float(6.67e-11) * (*planets)[0].mass * glm::linearRand<float>(0.7f, 1.4f) / glm::length((*planets)[i].pos_0)) * glm::cross(glm::normalize((*planets)[i].pos_0), glm::vec3(0.0f, 0.0f, 1.0f));
		//(*planets)[i].mass = glm::linearRand<float>(0.1f, 2.0f);
		(*planets)[i].mass = (*planets)[0].mass * glm::pow((*planets)[i].radius / (*planets)[0].radius, 3.0f);
		(*planets)[i].axis = glm::vec3(glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1));
		(*planets)[i].theta = 0.0f;
	}

	for (uint32_t i = 0; i < planets->size(); i++) {
		(*planets)[i].pos_1 = (*planets)[i].pos_0;
		(*planets)[i].vel_1 = (*planets)[i].vel_0;
		(*planets)[i].pos_2 = (*planets)[i].pos_0;
		(*planets)[i].vel_2 = (*planets)[i].vel_0;
	}

}
void particleRasterizer::initRast_B() {
	createDescriptorSets();
	createPipeline();
}

void particleRasterizer::createPipeline() {

	//first create shader modules
	std::array<VkShaderModule, 4> shaderModules;
	std::array<VkPipelineShaderStageCreateInfo, 4> shaderStages;
	std::array<VkShaderStageFlagBits, 4> flagBits{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT,VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };


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
	//rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	////rasterizer.polygonMode = VK_POLYGON_MODE_LINE;

	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //for point rendering
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaSamples;

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
	camera.size = sizeof(glm::mat4);
	camera.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &camera;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer pipelineLayout"); }

	std::array<VkPipelineShaderStageCreateInfo, 2> pipelineStages = { shaderStages[0], shaderStages[1] };

	VkGraphicsPipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = static_cast<uint32_t>(pipelineStages.size());
	createInfo.pStages = pipelineStages.data();
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

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer pipeline"); }

	//now create line pipeline
	auto lineBindingDescription = LineVertex::getBindingDescription();
	auto lineAttributeDescriptions = LineVertex::getAttributeDescriptions();

	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(lineAttributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &lineBindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = lineAttributeDescriptions.data();

	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	pipelineStages = { shaderStages[2], shaderStages[3] };

	createInfo.pStages = pipelineStages.data();

	//depthStencil.depthTestEnable = VK_FALSE;
	//depthStencil.depthWriteEnable = VK_FALSE;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &linePipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer linePipeline"); }

	for (uint32_t i = 0; i < shaderModules.size(); i++) {
		vkDestroyShaderModule(device, shaderModules[i], nullptr);
	}
}

void particleRasterizer::createDescriptorSets() {
	//start by creating the descriptor set layout
	VkDescriptorSetLayoutBinding ubo{};
	ubo.binding = 0;
	ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo.descriptorCount = 1;
	ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding lInfo{};
	lInfo.binding = 1;
	lInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lInfo.descriptorCount = 1;
	lInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	lInfo.pImmutableSamplers = nullptr;


	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo, lInfo };

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizerDescriptorSetLayout"); }
	
	//now allocate the descriptor sets

	std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { descriptorSetLayout, descriptorSetLayout, descriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate particleRasterizer descriptor sets"); }

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = i * sizeof(UniformBufferObject);
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorBufferInfo lIBuffer{};
		lIBuffer.buffer = *satLineInfoBuffer;
		lIBuffer.offset = 0;
		lIBuffer.range = sizeof(LineInfo) * SATELLITE_COUNT;


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
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &lIBuffer;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void particleRasterizer::createBuffers() {
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
		


		if (vkCreateBuffer(device, &vertexInfo, nullptr, &vertexBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer vertexBuffer"); }
		if (vkCreateBuffer(device, &indexInfo, nullptr, &indexBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer indexBuffer"); }

		VkMemoryRequirements vertexMem;
		VkMemoryRequirements indexMem;

		vkGetBufferMemoryRequirements(device, vertexBuffers[i], &vertexMem);
		vkGetBufferMemoryRequirements(device, indexBuffers[i], &indexMem);

		if (i == 0) {
			vertexRequirements.requirements = vertexMem;
			indexRequirements.requirements = indexMem;
			vertexOffsets[i] = 0;
			indexOffsets[i] = 0;
		}
		else {
			vertexOffsets[i] = static_cast<uint32_t>(vertexRequirements.requirements.size);
			indexOffsets[i] = static_cast<uint32_t>(indexRequirements.requirements.size);

			vertexRequirements.requirements.size += vertexMem.size;
			indexRequirements.requirements.size += indexMem.size;
		}
	}
	vertexOffsets.push_back(static_cast<uint32_t>(vertexRequirements.requirements.size));
	indexOffsets.push_back(static_cast<uint32_t>(indexRequirements.requirements.size));

	//now make line buffer
	lineBuffers.resize(MAX_PLANET_ARRAY_SIZE);
	lineVertices.resize(MAX_PLANET_ARRAY_SIZE);
	for (uint32_t i = 0; i < lineBuffers.size(); i++) {
		vertexInfo.size = LINE_VERTEX_COUNT * sizeof(LineVertex) * FRAMES_IN_FLIGHT;

		if (vkCreateBuffer(device, &vertexInfo, nullptr, &lineBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer lineBuffer"); }

		VkMemoryRequirements lineMem;

		vkGetBufferMemoryRequirements(device, lineBuffers[i], &lineMem);

		if (i == 0) {
			lineRequirements.requirements = lineMem;
			lineBufferOffsets.push_back(0);
		}
		else {
			lineBufferOffsets.push_back(static_cast<uint32_t>(lineRequirements.requirements.size));
			lineRequirements.requirements.size += lineMem.size;
		}
	}
	lineBufferOffsets.push_back(static_cast<uint32_t>(lineRequirements.requirements.size));



	//now make uniform buffer
	VkDeviceSize bufferSize = sizeof(UniformBufferObject) * FRAMES_IN_FLIGHT;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer uniform buffers"); }

	
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements.requirements);

	vertexRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	indexRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	lineRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uniformRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

}

void particleRasterizer::initMemory(MemInit* detPtr) {
	std::array<MemInit, 4> details;
	memcpy(details.data(), detPtr, details.size() * sizeof(MemInit));

	vertexMemory = details[0];
	indexMemory = details[1];
	uniformBufferMemory = details[2];
	lineMemory = details[3];
	
	//std::cout << vertexMemory.range << " | " << vertexRequirements.requirements.size << std::endl;
	//std::cout << indexMemory.range << " | " << indexRequirements.requirements.size << std::endl;
	//std::cout << uniformBufferMemory.range << " | " << uniformRequirements.requirements.size << std::endl;

	for (size_t i = 0; i < meshes.size(); i++) {
		vkBindBufferMemory(device, vertexBuffers[i], vertexMemory.memory, vertexMemory.offset + vertexOffsets[i]);
		vkBindBufferMemory(device, indexBuffers[i], indexMemory.memory, indexMemory.offset + indexOffsets[i]);
	}
	for (uint32_t i = 0; i < lineBuffers.size(); i++) {
		vkBindBufferMemory(device, lineBuffers[i], lineMemory.memory, lineMemory.offset + lineBufferOffsets[i]);
	}
	vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory.memory, uniformBufferMemory.offset);

	void* data;

	vkMapMemory(device, uniformBufferMemory.memory, uniformBufferMemory.offset, uniformBufferMemory.range, 0, &data);

	uniformBufferMapped = static_cast<char*>(data);

	vkMapMemory(device, lineMemory.memory, lineMemory.offset, lineMemory.range, 0, &data);

	char* lineMapped = static_cast<char*>(data);
	lineBuffersMapped.resize(lineBuffers.size());
	for (uint32_t i = 0; i < lineBuffers.size(); i++) {
		lineBuffersMapped[i] = lineMapped + lineBufferOffsets[i];
	}

	//std::cout << static_cast<char*>(data) << std::endl;
	//std::cout << uniformBufferMapped << std::endl;
	//std::cout << uniformBufferMapped + sizeof(UniformBufferObject) << std::endl;

	//char* data2 = uniformBufferMapped + sizeof(UniformBufferObject);
	//std::cout << sizeof(UniformBufferObject) << std::endl;
	//std::cout << static_cast<void*>(data2) << std::endl;
}

void particleRasterizer::initBufferData_A(MemoryDetails* stagingRequirements) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = maxBufferSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &createInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizer staging buffer"); }

	vkGetBufferMemoryRequirements(device, stagingBuffer, &(stagingRequirements->requirements));

	stagingRequirements->flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}
void particleRasterizer::initBufferData_B(VkCommandBuffer transferCommandBuffer, VkQueue transferQueue, MemInit memory) {
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
	//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;
	
	void* data;
	vkMapMemory(device, stagingMemory.memory, stagingMemory.offset, stagingMemory.range, 0, &data);

	for (size_t i = 0; i < meshes.size(); i++) {
		//vertices
		//step 1 copy data to to buffer
		memcpy(data, meshes[i].vertices->data(), meshes[i].vertices->size() * sizeof(Vertex));
		std::vector<Vertex> newVert(meshes[i].vertices->size());
		memcpy(newVert.data(), meshes[i].vertices->data(), meshes[i].vertices->size() * sizeof(Vertex));

		//std::cout << *reinterpret_cast<float*>(data) << std::endl;
		//std::cout << *(reinterpret_cast<float*>(data) + 8) << std::endl;

		if(vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin transfer command buffer");
		};
		VkBufferCopy copy{};
		copy.size = meshes[i].vertices->size() * sizeof(Vertex);
		copy.srcOffset = 0;
		copy.dstOffset = vertexOffsets[i];
		
		//step 2 perform copy
		vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, (vertexBuffers[i]), 1, &copy);
		if(vkEndCommandBuffer(transferCommandBuffer)!=VK_SUCCESS){
		throw std::runtime_error("Failed to end transfer command buffer");
	};
		VkMappedMemoryRange memoryRange{};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = stagingMemory.memory;
		memoryRange.offset = stagingMemory.offset;
		memoryRange.size = stagingMemory.range;

		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		//vkFlushMappedMemoryRanges(device, 1, &memoryRange);
		vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
		//std::cout<<"Fence: "<<vkGetFenceStatus(device, transferFence);
		vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		vkResetFences(device, 1, &transferFence);
		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		vkResetCommandBuffer(transferCommandBuffer, 0);
		//std::cout << *(reinterpret_cast<float*>(data) + 8) << std::endl;
	}
	//for (size_t i = 0; i < meshes.size(); i++) {
	//	//indices
	//	//step 1 copy data to to buffer
	//	memcpy(data, meshes[i].indices->data(), meshes[i].indices->size() * sizeof(uint16_t));
	//	
	//	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
	//	VkBufferCopy copy{};
	//	copy.size = meshes[i].indices->size() * sizeof(uint16_t);
	//	copy.srcOffset = 0;
	//	copy.dstOffset = 0;
	//	//step 2 perform copy
	//	vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, vertexBuffers[i], 1, &copy);
	//	vkEndCommandBuffer(transferCommandBuffer);
	//	vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
	//	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	//	vkResetCommandBuffer(transferCommandBuffer, 0);
	//}

	for (size_t i = 0; i < meshes.size(); i++) {
		//vertices
		//step 1 copy data to to buffer
		memcpy(data, meshes[i].indices->data(), meshes[i].indices->size() * sizeof(uint16_t));
		std::vector<uint16_t> newVert(meshes[i].indices->size());
		memcpy(newVert.data(), meshes[i].indices->data(), meshes[i].indices->size() * sizeof(uint16_t));

		//std::cout << *reinterpret_cast<float*>(data) << std::endl;
		//std::cout << *(reinterpret_cast<float*>(data) + 8) << std::endl;

		if (vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin transfer command buffer");
		};
		VkBufferCopy copy{};
		copy.size = meshes[i].indices->size() * sizeof(uint16_t);
		copy.srcOffset = 0;
		copy.dstOffset = indexOffsets[i];

		//step 2 perform copy
		vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, (indexBuffers[i]), 1, &copy);
		if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to end transfer command buffer");
		};
		VkMappedMemoryRange memoryRange{};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = stagingMemory.memory;
		memoryRange.offset = stagingMemory.offset;
		memoryRange.size = stagingMemory.range;

		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		//vkFlushMappedMemoryRanges(device, 1, &memoryRange);
		vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		vkResetFences(device, 1, &transferFence);
		//std::cout << "Fence: " << vkGetFenceStatus(device, transferFence);
		vkResetCommandBuffer(transferCommandBuffer, 0);
		//std::cout << *(reinterpret_cast<float*>(data) + 8) << std::endl;
	}
	vkUnmapMemory(device, stagingMemory.memory);

	stagingMemory = {};


	//cleanup resources



	vkDestroyFence(device, transferFence, nullptr);

	//vkDestroyBuffer(device, stagingBuffer, nullptr);
	
}
void particleRasterizer::drawObjects(VkCommandBuffer commandBuffer, uint32_t frameIndex, UniformBufferObject ubo, float dt) {
	glm::mat4 model(1);
	dt = dt * 1e3f;
	for (uint32_t i = 0; i < planets->size(); i++) {
		ubo.models[i] = (*planets)[i].getModelMatrix(1.0f);
		//(*planets)[i].pos_0 += dt * (*planets)[i].vel_0;
		//for (uint32_t j = 0; j < planets->size(); j++) {
		//	if (i == j) {
		//		continue;
		//	}
		//	else {
		//		glm::vec3 sep = (*planets)[j].pos_0 - (*planets)[i].pos_0;
		//		float FMult = 6.67e-11 * pow(glm::length(sep), -3.0f) * (*planets)[j].mass;
		//		if (i == 0) {
		//			FMult = 0.0f;
		//		}
		//		(*planets)[i].vel_0 += FMult * sep * dt;
		//	}
		//}
		//(*planets)[i].theta += dt / 1e4;

		if (lineFrame == 0) {
			LineVertex newVertex{};
			newVertex.pos = (*planets)[i].pos_0;
			newVertex.baseColour = glm::vec3(0.0f, 0.0f, 1.0f);
			newVertex.intColour = glm::vec3(0.0f, 1.0f, 0.0f);
			newVertex.finColour = glm::vec3(0.0f, 0.0f, 0.0f);
			lineVertices[i][lineCursor] = newVertex;
		}
	}
	glm::vec4 unClipPos = ubo.proj * ubo.view * ubo.models[0] * glm::vec4((*meshes[0].vertices)[5].pos, 1.0f);
	glm::vec4 clipPos = unClipPos / unClipPos.w;

	//std::cout << dt<<" | "<<glm::to_string((*planets)[1].pos) << " | " << glm::to_string((*planets)[1].vel) << std::endl;
	//std::cout << glm::to_string(unClipPos) <<" | "<<glm::to_string(clipPos) << std::endl/* << glm::to_string(ubo.proj * ubo.view * ubo.models[1]) << std::endl*/;

	//render pass is already going so start by binding pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

	void* mappedAdress = uniformBufferMapped + (sizeof(UniformBufferObject) * frameIndex);

	memcpy(mappedAdress, &ubo, sizeof(UniformBufferObject));
	
	//then for each model
	VkDeviceSize offsets[] = { 0 };

	for (size_t i = 0; i < meshes.size(); i++) {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[i], offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffers[i], 0, VK_INDEX_TYPE_UINT16);
		//for single model
		vkCmdDrawIndexed(commandBuffer, meshes[i].indexCount, static_cast<uint32_t>(planets->size()), 0, 0, 0);
		//for instanced model everywhere
		//vkCmdDrawIndexed(commandBuffer, meshes[i].indexCount, particleCount, 0, 0, 0);
		//vkCmdDraw(commandBuffer, particleCount, 1, particleCount * frameIndex, 0);
	}
	if (lineFrame == 0) {
		lineCursor = (lineCursor + 1) % lineVertices[0].size();
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
	for (uint32_t i = 0; i < lineBuffers.size(); i++) {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &lineBuffers[i], offsets);
		model[0][0] = float(frameIndex) * float(static_cast<uint32_t>(lineVertices[i].size()));
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
		//vkCmdDraw(commandBuffer, lineSegments, 1, (lineVertices[i].size() * (frameIndex + 1)) - lineSegments, 0);
		////copy data to vertex buffer;
		//
		//
		//
		////std::cout << frameIndex << " _ " << i << " _ ";
		////std::cout << (void*)(lineBuffersMapped[i] + lineVertices[i].size() * sizeof(LineVertex) * frameIndex )<< " | ";
		////std::cout << (void*)(lineVertices[i].data() + lineCursor * sizeof(LineVertex)) << " | ";
		////std::cout << (void*)((lineVertices[i].size() - lineCursor) * sizeof(LineVertex) )<< "\t|\t";


		////std::cout << (void*)(lineBuffersMapped[i] + lineVertices[i].size() * sizeof(LineVertex) * frameIndex + lineCursor * sizeof(LineVertex)) << " | ";
		////std::cout << (void*)lineVertices.data() << " | ";
		////std::cout << (void*)(lineCursor * sizeof(LineVertex)) << std::endl;


		//memcpy(lineBuffersMapped[i] + lineVertices[i].size() * sizeof(LineVertex) * frameIndex + (lineVertices[i].size() - lineCursor) * sizeof(LineVertex), lineVertices[i].data(), lineCursor * sizeof(LineVertex)); //copy data before cursor position to back of buffer
		//memcpy(lineBuffersMapped[i] + lineVertices[i].size() * sizeof(LineVertex) * frameIndex, (char*)lineVertices[i].data() + lineCursor * sizeof(LineVertex), (lineVertices[i].size() - lineCursor) * sizeof(LineVertex)); //copy data after cursor position to front of buffer
		//

		memcpy(lineBuffersMapped[i] + lineVertices[i].size() * sizeof(LineVertex) * frameIndex, lineVertices[i].data(), lineVertices[i].size() * sizeof(LineVertex));

		model[0][0] = (float(frameIndex) - 1.0f) * lineVertices[i].size() + lineCursor;
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
		vkCmdDraw(commandBuffer, lineCursor, 1, static_cast<uint32_t>(lineVertices[i].size())*frameIndex, 0);
		
		if (lineSegments == 1024) {
			model[0][0] = float(frameIndex) * float(static_cast<uint32_t>(lineVertices[i].size())) + float(lineCursor);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineVertices[i].size()) - lineCursor, 1, lineCursor + static_cast<uint32_t>(lineVertices[i].size())*frameIndex, 0);
		}
	}
	//now draw satellite lines

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, satLineBuffer, offsets);
	for (uint32_t i = 0; i < SATELLITE_COUNT; i++) {
		model[0][0] = (float(i)-1.0f) * LINE_VERTEX_COUNT + *satLineCursor;
		model[0][1] = float(i);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
		vkCmdDraw(commandBuffer, *satLineCursor, 1, i* LINE_VERTEX_COUNT, 0);

		if (*satLineSegments == 1024) {
			model[0][0] = float(i) * float(LINE_VERTEX_COUNT) + float(*satLineCursor);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
			vkCmdDraw(commandBuffer, LINE_VERTEX_COUNT - *satLineCursor, 1, i* LINE_VERTEX_COUNT + *satLineCursor, 0);
		}
	}






	if (lineFrame == 0) {
		/*lineCursor = (lineCursor + 1) % lineVertices[0].size();*/
		lineSegments = (lineSegments < 1024) ? lineSegments + 1 : 1024;
	}

	lineFrame = (lineFrame + 1) % 60;
}	

void particleRasterizer::getMemoryRequirements(std::vector<MemoryDetails>* mem, std::vector<uint16_t>* count) {
	mem->push_back(vertexRequirements);
	mem->push_back(indexRequirements);
	mem->push_back(uniformRequirements);
	mem->push_back(lineRequirements);
	count->push_back(4);
}
void particleRasterizer::setExternalPtrs(SatExternalMembers details) {
	satLineBuffer = details.lineBuffer;
	satLineCursor = details.lineCursor;
	satLineSegments = details.lineSegments;
	satLineInfoBuffer = details.lineInfoBuffer;
}

void particleRasterizer::cleanup() {
	for (size_t i = 0; i < meshes.size(); i++) {
		vkDestroyBuffer(device, vertexBuffers[i], nullptr);
		vkDestroyBuffer(device, indexBuffers[i], nullptr);
	}
	for (size_t i = 0; i < lineBuffers.size(); i++) {
		vkDestroyBuffer(device, lineBuffers[i], nullptr);
	}
	vkUnmapMemory(device, uniformBufferMemory.memory);
	vkUnmapMemory(device, lineMemory.memory);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

	//vkFreeDescriptorSets(device, descriptorPool, descriptorSets.size(), descriptorSets.data());

	vkDestroyPipeline(device, linePipeline, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}