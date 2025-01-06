#include "satelliteEngine.h"

void SatelliteEngine::initSatEngine_A(SatInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	commandPool = details.commandPool;

	memProperties = details.memProperties;

	planets = details.planets;

	createBuffers();
}

void SatelliteEngine::createBuffers() {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	bufferInfo.size = LINE_VERTEX_COUNT * sizeof(LineVertex) * MAX_PLANET_ARRAY_SIZE;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &lineBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Satellite line buffer"); }
	vkGetBufferMemoryRequirements(device, lineBuffer, &storageRequirements.requirements);

	bufferInfo.size = SATELLITE_COUNT * sizeof(Satellite);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	satSize = bufferInfo.size;

	for (uint32_t i = 0; i < satBuffers.size(); i++) { if (vkCreateBuffer(device, &bufferInfo, nullptr, &satBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create SatelliteBuffers"); } }
	storageRequirements.requirements.size += bufferInfo.size * satBuffers.size();
	
	bufferInfo.size = FRAMES_IN_FLIGHT * planets->size() * sizeof(Planet);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	uniformSize = bufferInfo.size / FRAMES_IN_FLIGHT;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Satellite unifomr buffer"); }
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements.requirements);

	storageRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uniformRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

void SatelliteEngine::createDescriptorSets() {
	VkDescriptorSetLayoutBinding ubo{};
	ubo.binding = 0;
	ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo.descriptorCount = 1;
	ubo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	ubo.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding iBuffer{};
	iBuffer.binding = 1;
	iBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	iBuffer.descriptorCount = 1;
	iBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	iBuffer.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding oBuffer{};
	oBuffer.binding = 2;
	oBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	oBuffer.descriptorCount = 1;
	oBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	oBuffer.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { ubo, iBuffer, oBuffer };

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
		bufferInfo.offset = i * uniformSize;
		bufferInfo.range = uniformSize;

		VkDescriptorBufferInfo iInfo{};
		iInfo.buffer = satBuffers[i];
		iInfo.offset = 0;
		iInfo.range = satSize;

		VkDescriptorBufferInfo oInfo{};
		oInfo.buffer = satBuffers[(i + 1) % FRAMES_IN_FLIGHT];
		oInfo.offset = 0;
		oInfo.range = satSize;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

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
		descriptorWrites[1].pBufferInfo = &iInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &iInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void SatelliteEngine::createPipeline() {
	std::vector<VkShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderModules.resize(shaderFiles.size());
	shaderStages.resize(shaderFiles.size());

	for (uint32_t i = 0; i < shaderFiles.size(); i++) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode[i]->size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode[i]->data());
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModules[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite shader modules"); }
	}
	VkPushConstantRange range{};
	range.size = sizeof(SatPushConstants);
	range.offset = 0;
	range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &range;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite pipeline layout"); }

	VkComputePipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.layout = pipelineLayout;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.flags = 0;

	createInfo.stage = shaderStages[0];
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite pipeline"); }

	createInfo.stage = shaderStages[1];
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &linePipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite pipeline"); }

	for (uint32_t i = 0; i < shaderModules.size(); i++) {
		vkDestroyShaderModule(device, shaderModules[i], nullptr);
	}
}