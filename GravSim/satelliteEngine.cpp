#include "satelliteEngine.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>


void SatelliteEngine::initSatEngine_A(SatInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;

	memProperties = details.memProperties;

	planets = details.planets;

	settings = details.settings;

	memcpy(shaderCode.data(), details.shaderCode.data(), shaderCode.size() * sizeof(shaderCode[0]));

	createBuffers();

	satUBO = new SatUniformBuffer;
}
void SatelliteEngine::initSatEngine_B() {
	createDescriptorSets();
	createPipeline();
}

void SatelliteEngine::createBuffers() {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	bufferInfo.size = LINE_VERTEX_COUNT * sizeof(LineVertex) * SATELLITE_COUNT;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	lineSize = bufferInfo.size;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &lineBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Satellite line buffer"); }
	vkGetBufferMemoryRequirements(device, lineBuffer, &lineRequirements.requirements);

	bufferInfo.size = SATELLITE_COUNT * sizeof(Satellite);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	satSize = bufferInfo.size;

	for (uint32_t i = 0; i < satBuffers.size(); i++) { if (vkCreateBuffer(device, &bufferInfo, nullptr, &satBuffers[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create SatelliteBuffers"); } }
	vkGetBufferMemoryRequirements(device, satBuffers[0], &satRequirements.requirements);
	satRequirements.requirements.size *= satBuffers.size();
	
	bufferInfo.size = FRAMES_IN_FLIGHT * sizeof(SatUniformBuffer);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	uniformSize = bufferInfo.size / FRAMES_IN_FLIGHT;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Satellite uniform buffer"); }
	vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformRequirements.requirements);

	bufferInfo.size = SATELLITE_COUNT * sizeof(LineInfo);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	lineInfoSize = bufferInfo.size;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &lineInfoBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Satellite lineInfo buffer"); }
	vkGetBufferMemoryRequirements(device, lineInfoBuffer, &lineInfoRequirements.requirements);

	bufferInfo.size = fieldMeshResMajor * fieldMeshResMinor * 2 * fieldMeshResMajor * sizeof(LineVertex);
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	fieldMeshSize = bufferInfo.size;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &fieldMeshBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create field mesh buffer"); }
	vkGetBufferMemoryRequirements(device, fieldMeshBuffer, &fieldMeshRequirements.requirements);


	lineRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	satRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	uniformRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	lineInfoRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	fieldMeshRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
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

	VkDescriptorSetLayoutBinding lBuffer{};
	lBuffer.binding = 3;
	lBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lBuffer.descriptorCount = 1;
	lBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	lBuffer.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding lIBuffer{};
	lIBuffer.binding = 4;
	lIBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lIBuffer.descriptorCount = 1;
	lIBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	lIBuffer.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding mBuffer{};
	mBuffer.binding = 5;
	mBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	mBuffer.descriptorCount = 1;
	mBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	mBuffer.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 6> bindings = { ubo, iBuffer, oBuffer,lBuffer,lIBuffer,mBuffer };

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create particleRasterizerDescriptorSetLayout"); }

	//now allocate the descriptor sets

	std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT * 2> layouts = { descriptorSetLayout, descriptorSetLayout, descriptorSetLayout, descriptorSetLayout, descriptorSetLayout, descriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate particleRasterizer descriptor sets"); }

	for (size_t i = 0; i < FRAMES_IN_FLIGHT * 2; i += 2) {

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = (i / 2) * uniformSize;
		bufferInfo.range = uniformSize;

		VkDescriptorBufferInfo iInfo{};
		iInfo.buffer = satBuffers[0];
		iInfo.offset = 0;
		iInfo.range = satSize;

		VkDescriptorBufferInfo oInfo{};
		oInfo.buffer = satBuffers[1];
		oInfo.offset = 0;
		oInfo.range = satSize;

		VkDescriptorBufferInfo lInfo{};
		lInfo.buffer = lineBuffer;
		lInfo.offset = 0;
		lInfo.range = lineSize;

		VkDescriptorBufferInfo lIInfo{};
		lIInfo.buffer = lineInfoBuffer;
		lIInfo.offset = 0;
		lIInfo.range = lineInfoSize;

		VkDescriptorBufferInfo mInfo{};
		mInfo.buffer = fieldMeshBuffer;
		mInfo.offset = 0;
		mInfo.range = fieldMeshSize;

		std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

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
		descriptorWrites[2].pBufferInfo = &oInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &lInfo;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = descriptorSets[i];
		descriptorWrites[4].dstBinding = 4;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pBufferInfo = &lIInfo;

		descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[5].dstSet = descriptorSets[i];
		descriptorWrites[5].dstBinding = 5;
		descriptorWrites[5].dstArrayElement = 0;
		descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[5].descriptorCount = 1;
		descriptorWrites[5].pBufferInfo = &mInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);


		iInfo.buffer = satBuffers[1];
		oInfo.buffer = satBuffers[0];


		descriptorWrites[0].dstSet = descriptorSets[i + 1];
		descriptorWrites[1].dstSet = descriptorSets[i + 1];
		descriptorWrites[2].dstSet = descriptorSets[i + 1];
		descriptorWrites[3].dstSet = descriptorSets[i + 1];
		descriptorWrites[4].dstSet = descriptorSets[i + 1];
		descriptorWrites[5].dstSet = descriptorSets[i + 1];

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void SatelliteEngine::createPipeline() {
	std::vector<VkShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderModules.resize(shaderFiles.size());
	shaderStages.resize(shaderFiles.size());

	SatSpecConstants specConstantsData{};
	specConstantsData.G = CONSTANT_G;
	specConstantsData.MAX_PLANET_ARRAY_SIZE = MAX_PLANET_ARRAY_SIZE;
	specConstantsData.SATELLITE_COUNT = SATELLITE_COUNT;
	specConstantsData.LINE_VERTEX_COUNT = LINE_VERTEX_COUNT;
	specConstantsData.COMPUTE_STEPS_PER_FRAME = COMPUTE_STEPS_PER_FRAME;
	specConstantsData.MESH_STEPS_MAJOR = fieldMeshResMajor;
	specConstantsData.MESH_STEPS_MINOR = fieldMeshResMinor;
	specConstantsData.SATELLITES_PER_SHADER = SATELLITES_PER_SHADER;

	std::array<VkSpecializationMapEntry, 8> specEntries{};
	specEntries[0].constantID = 0;
	specEntries[0].offset = offsetof(SatSpecConstants, G);
	specEntries[0].size = sizeof(specConstantsData.G);
	specEntries[1].constantID = 1;
	specEntries[1].offset = offsetof(SatSpecConstants, MAX_PLANET_ARRAY_SIZE);
	specEntries[1].size = sizeof(specConstantsData.MAX_PLANET_ARRAY_SIZE);
	specEntries[2].constantID = 2;
	specEntries[2].offset = offsetof(SatSpecConstants, SATELLITE_COUNT);
	specEntries[2].size = sizeof(specConstantsData.SATELLITE_COUNT);
	specEntries[3].constantID = 3;
	specEntries[3].offset = offsetof(SatSpecConstants, LINE_VERTEX_COUNT);
	specEntries[3].size = sizeof(specConstantsData.LINE_VERTEX_COUNT);
	specEntries[4].constantID = 4;
	specEntries[4].offset = offsetof(SatSpecConstants, COMPUTE_STEPS_PER_FRAME);
	specEntries[4].size = sizeof(specConstantsData.COMPUTE_STEPS_PER_FRAME);
	specEntries[5].constantID = 5;
	specEntries[5].offset = offsetof(SatSpecConstants, MESH_STEPS_MAJOR);
	specEntries[5].size = sizeof(specConstantsData.MESH_STEPS_MAJOR);
	specEntries[6].constantID = 6;
	specEntries[6].offset = offsetof(SatSpecConstants, MESH_STEPS_MINOR);
	specEntries[6].size = sizeof(specConstantsData.MESH_STEPS_MINOR);
	specEntries[7].constantID = 7;
	specEntries[7].offset = offsetof(SatSpecConstants, SATELLITES_PER_SHADER);
	specEntries[7].size = sizeof(specConstantsData.SATELLITES_PER_SHADER);

	VkSpecializationInfo specInfo{};
	specInfo.mapEntryCount = static_cast<uint32_t>(specEntries.size());
	specInfo.pMapEntries = specEntries.data();
	specInfo.dataSize = sizeof(SatSpecConstants);
	specInfo.pData = &specConstantsData;

	for (uint32_t i = 0; i < shaderFiles.size(); i++) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode[i]->size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode[i]->data());
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModules[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite shader modules"); }
	
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.pSpecializationInfo = &specInfo;
		stageInfo.pName = "main";
		stageInfo.module = shaderModules[i];
		shaderStages[i] = stageInfo;
	
	
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

	createInfo.stage = shaderStages[2];
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &meshPipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite pipeline"); }


	for (uint32_t i = 0; i < shaderModules.size(); i++) {
		vkDestroyShaderModule(device, shaderModules[i], nullptr);
	}
}


void SatelliteEngine::getMemoryRequirements(std::vector<MemoryDetails>* mem, std::vector<uint16_t>* count) {
	mem->push_back(lineRequirements);
	mem->push_back(satRequirements);
	mem->push_back(uniformRequirements);
	mem->push_back(lineInfoRequirements);
	mem->push_back(fieldMeshRequirements);
	count->push_back(5);
}
void SatelliteEngine::initMemory(MemInit* detPtr) {
	std::array<MemInit, 5> details;
	memcpy(details.data(), detPtr, details.size() * sizeof(MemInit));

	lineMemory = details[0];
	satMemory = details[1];
	uniformMemory = details[2];
	lineInfoMemory = details[3];
	fieldMeshMemory = details[4];

	vkBindBufferMemory(device, lineBuffer, lineMemory.memory, lineMemory.offset);

	for (uint32_t i = 0; i < satBuffers.size(); i++) { vkBindBufferMemory(device, satBuffers[i], satMemory.memory, satMemory.offset + i * satSize); }

	vkBindBufferMemory(device, uniformBuffer, uniformMemory.memory, uniformMemory.offset);
	
	void* data;

	vkMapMemory(device, uniformMemory.memory, uniformMemory.offset, uniformMemory.range, 0, &data);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		uniformBuffersMapped[i] = reinterpret_cast<char*>(data) + i * uniformSize;
	}

	vkBindBufferMemory(device, lineInfoBuffer, lineInfoMemory.memory, lineInfoMemory.offset);

	vkBindBufferMemory(device, fieldMeshBuffer, fieldMeshMemory.memory, fieldMeshMemory.offset);

}
void SatelliteEngine::initBufferData_A(MemoryDetails* stagingRequiements) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = satRequirements.requirements.size;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	if (vkCreateBuffer(device, &createInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create satellite staging buffer"); }
	vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingRequiements->requirements);
	stagingRequiements->flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}
void SatelliteEngine::initBufferData_B(VkCommandBuffer transferCommandBuffer, VkQueue transferQueue, MemInit memory) {
	vkBindBufferMemory(device, stagingBuffer, memory.memory, memory.offset);
	void* data;
	vkMapMemory(device, memory.memory, memory.offset, memory.range, 0, &data);


	//OLD SAT INITALISATION
	//uint32_t index = 0;
	//for (uint32_t i = 0; i < 4; i++) {
	//	for (uint32_t j = 0; j < 16; j++) {
	//		Satellite newSat{};
	//		newSat.pos = glm::vec3(0.0f, 4.6e8, i*2e7);
	//		newSat.mass = 1.0f;
	//		newSat.vel = ((float(j) / 256.0f) + 0.4f) * glm::sqrt(float(CONSTANT_G) * (*planets)[0].mass / glm::length(newSat.pos)) * glm::cross(glm::normalize(-newSat.pos), glm::vec3(0.0f, 0.0f, 1.0f));
	//		satData[index] = newSat;
	//		index++;
	//	}
	//}

	////NEW SATS
	////4 sets of 8 velocities in LEO
	//createInitialSatellitesBase(&satData[0], 8, 4, 200e3f, 1.0f, 0.2f, 0.0f, 0);
	////4 sets of 4 velocities in MEO
	//createInitialSatellitesBase(&satData[32], 4, 4, 1e7f, 0.8f, 0.2f, 0.0f, 0);
	////4 sets of 4 velocities in LLO
	//createInitialSatellitesBase(&satData[48], 4, 4, 100e3f, 1.0f, 0.2f, 0.0f, 1);

	//NEW SATS
	//4 sets of 16 velocities in LEO
	createInitialSatellitesBase(&satData[0], 4, 16, 200e3f, 1.0f, 0.017f, 0.0f, 1);

	createInitialSatellitesOffset(&satData[64], 6, 32, 400e3, 1.35, 1.5, 0.6, 0);

	createInitialSatellitesOffset(&satData[256], 60, 64, 1e4, 1.35, 1.6, 0.7, 0);

	memcpy(data, satData.data(), satData.size() * sizeof(Satellite));

	VkFence transferFence;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(device, &fenceInfo, nullptr, &transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to create transfer fence for GravDataSync"); }

	//record copy operation
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer;

	VkBufferCopy cpy{};
	cpy.srcOffset = 0;
	cpy.dstOffset = 0;
	cpy.size = satData.size() * sizeof(Satellite);

	if (vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin transfer command buffer"); }
	for (uint32_t i = 0; i < satBuffers.size(); i++) { vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, satBuffers[i], 1, &cpy); }
	if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to end transfer command buffer"); }
	if (vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to submit transfer command buffer"); }
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	vkUnmapMemory(device, memory.memory);


	vkDestroyFence(device, transferFence, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

}

void SatelliteEngine::simulateSats(VkCommandBuffer commandBuffer, uint32_t frameIndex, double dt) {

	dt = 1.0 / 400.0;
	dt *= 1e3;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	
	
	SatPushConstants satPC{};
	satPC.deltaTime = dt / COMPUTE_STEPS_PER_FRAME;

	VkBufferMemoryBarrier bar1{};
	bar1.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bar1.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bar1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bar1.offset = 0;
	bar1.size = satSize;
	bar1.buffer = satBuffers[0];
	
	VkBufferMemoryBarrier bar2{};
	bar2.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bar2.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bar2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bar2.offset = 0;
	bar2.size = satSize;
	bar2.buffer = satBuffers[1];

	std::array<VkBufferMemoryBarrier, 2> barriers = { bar1, bar2 };

	uint32_t shaderDispatches = uint32_t(ceil((float(SATELLITE_COUNT)/float(SATELLITES_PER_SHADER)) / 1024.0));

	for (uint32_t i = 0; i < COMPUTE_STEPS_PER_FRAME; i++) {
		updatePlanets(0.5 * satPC.deltaTime);
		updatePlanets(0.5 * satPC.deltaTime);
		memcpy(satUBO->planetData[i].data(), planets->data(), planets->size() * sizeof(Planet));

		satPC.planetIndex = i;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[2 * frameIndex + i % 2], 0, nullptr);

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SatPushConstants), &satPC);

		vkCmdDispatch(commandBuffer, shaderDispatches, 1, 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(barriers.size()), barriers.data(), 0, nullptr);
	}

	memcpy(uniformBuffersMapped[frameIndex], satUBO, sizeof(SatUniformBuffer));

	if (lineFrame == WRITE_FRAME) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, linePipeline);

		satPC.deltaTime = float(lineCursor);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SatPushConstants), &satPC);

		vkCmdDispatch(commandBuffer, shaderDispatches, 1, 1);

		if (settings->renderFieldMesh) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, meshPipeline);
			vkCmdDispatch(commandBuffer, fieldMeshResMajor * 2, 1, 1);
		}

		VkMemoryBarrier mem{};
		mem.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		mem.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		mem.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

		VkMemoryBarrier mem2{};
		mem2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		mem2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		mem2.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &mem, 0, nullptr, 0, nullptr);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &mem2, 0, nullptr, 0, nullptr);
		
		lineCursor = (lineCursor + 1) % LINE_VERTEX_COUNT;
		lineSegments = (lineSegments < 1024) ? lineSegments + 1 : 1024;
	}
	lineFrame = (lineFrame + 1) % FRAMES_PER_LINE;

}
SatExternalMembers SatelliteEngine::getSatellitePtrs() {
	SatExternalMembers data{};
	data.lineBuffer = &lineBuffer;
	data.lineCursor = &lineCursor;
	data.lineSegments = &lineSegments;
	data.lineInfoBuffer = &lineInfoBuffer;
	data.meshBuffer = &fieldMeshBuffer;
	return data;
}

void SatelliteEngine::createInitialSatellitesBase(void* ptr, uint32_t positions, uint32_t velocities, double baseHeight, double baseVelocity, double velocityStep, double inclination, uint32_t planetIndex) {
	Planet pl = (*planets)[planetIndex];
	double orbitRadius = pl.radius + baseHeight;
	double orbitVel = glm::sqrt(CONSTANT_G * pl.mass / orbitRadius);
	Satellite sat{};
	sat.mass = 0.0;
	double trueAnomaly = 0.0;
	glm::dvec3 radiusVec{};

	Satellite* satPtr = reinterpret_cast<Satellite*>(ptr);
	uint32_t satIndex = 0;

	for (uint32_t i = 0; i < positions; i++) {
		trueAnomaly = glm::two_pi<float>() * i / positions;
		radiusVec = orbitRadius * glm::dvec3(glm::cos(trueAnomaly), glm::sin(trueAnomaly), 0.0f);
		sat.pos = pl.pos_2 + radiusVec;
		for (uint32_t j = 0; j < velocities; j++) {
			sat.vel = glm::normalize(glm::cross(radiusVec, glm::dvec3(0.0f, 0.0f, 1.0f))) * orbitVel * (baseVelocity + glm::sqrt(double(j)) * velocityStep) + pl.vel_2;
			*(satPtr + satIndex) = sat;
			satIndex++;
		}
	}
}
void SatelliteEngine::createInitialSatellitesOffset(void* ptr, uint32_t positions, uint32_t velocities, double baseHeight, double minVelocity, double maxVelocity, double distFactor, uint32_t planetIndex) {
	Planet pl = (*planets)[planetIndex];
	double orbitRadius = pl.radius + baseHeight;
	double orbitVel = glm::sqrt(CONSTANT_G * pl.mass / orbitRadius);
	Satellite sat{};
	sat.mass = 0.0;
	double trueAnomaly = 0.0;
	glm::dvec3 radiusVec{};

	double velBase = minVelocity * orbitVel;
	double velStep = (maxVelocity - minVelocity) * orbitVel;

	Satellite* satPtr = reinterpret_cast<Satellite*>(ptr);
	uint32_t satIndex = 0;
	for (uint32_t i = 0; i < positions; i++) {
		trueAnomaly = glm::two_pi<float>() * i / positions;
		radiusVec = orbitRadius * glm::dvec3(glm::cos(trueAnomaly), glm::sin(trueAnomaly), 0.0f);
		sat.pos = pl.pos_2 + radiusVec;
		for (uint32_t j = 0; j < velocities; j++) {
			double velStepBase = (velocities > 1) ? (double(j) / double(velocities - 1)) : 0.0;
			double velStepFactor = std::pow(velStepBase, distFactor);
			double velMult = velBase + velStep * velStepFactor;
			sat.vel = glm::normalize(glm::cross(radiusVec, glm::dvec3(0.0, 0.0, 1.0))) * velMult + pl.vel_2;
			*(satPtr + satIndex) = sat;
			satIndex++;
		}
	}

}

void SatelliteEngine::updatePlanets(double dt) {
	//use RK-4 to update planets.
	//first copy current planet data to tempPlanets
	for (uint32_t i = 0; i < tempPlanets.size(); i++) {
		memcpy(tempPlanets[i].data(), planets->data(), planets->size() * sizeof(Planet));
	}
	//now update accel using 0th positions;
	updateAccelerations(0);
	//now setup next inputs;
	for (uint32_t i = 0; i < MAX_PLANET_ARRAY_SIZE; i++) {
		dx_1[i] = tempPlanets[0][i].vel_2 * dt;
		dv_1[i] = tempAccelerations[i] * dt;
		tempPlanets[1][i].pos_2 += 0.5 * dx_1[i];
		tempPlanets[1][i].vel_2 += 0.5 * dv_1[i];
	}
	updateAccelerations(1);

	for (uint32_t i = 0; i < MAX_PLANET_ARRAY_SIZE; i++) {
		dx_2[i] = tempPlanets[1][i].vel_2 * dt;
		dv_2[i] = tempAccelerations[i] * dt;
		tempPlanets[2][i].pos_2 += 0.5 * dx_2[i];
		tempPlanets[2][i].vel_2 += 0.5 * dv_2[i];
	}
	updateAccelerations(2);

	for (uint32_t i = 0; i < MAX_PLANET_ARRAY_SIZE; i++) {
		dx_3[i] = tempPlanets[2][i].vel_2 * dt;
		dv_3[i] = tempAccelerations[i] * dt;
		tempPlanets[3][i].pos_2 += 1.0 * dx_3[i];
		tempPlanets[3][i].vel_2 += 1.0 * dv_3[i];
	}

	updateAccelerations(3);
	for (uint32_t i = 0; i < MAX_PLANET_ARRAY_SIZE; i++) {
		dx_4[i] = tempPlanets[3][i].vel_2 * dt;
		dv_4[i] = tempAccelerations[i] * dt;
		dx[i] = (1.0 * dx_1[i] + 2.0 * dx_2[i] + 2.0 * dx_3[i] + 1.0 * dx_4[i]) / 6.0;
		dv[i] = (1.0 * dv_1[i] + 2.0 * dv_2[i] + 2.0 * dv_3[i] + 1.0 * dv_4[i]) / 6.0;
	}

	//LOCK EARTH IN PLACE
	//dx[0] = glm::vec3(0);
	//dv[0] = glm::vec3(0);
	
	//now update final planets;
	for (uint32_t i = 0; i < MAX_PLANET_ARRAY_SIZE; i++) {
		(*planets)[i].pos_0 = tempPlanets[0][i].pos_1;
		(*planets)[i].vel_0 = tempPlanets[0][i].vel_1;
		(*planets)[i].pos_1 = tempPlanets[0][i].pos_2;
		(*planets)[i].vel_1 = tempPlanets[0][i].vel_2;
		(*planets)[i].pos_2 = tempPlanets[0][i].pos_2 + dx[i];
		(*planets)[i].vel_2 = tempPlanets[0][i].vel_2 + dv[i];
		(*planets)[i].theta = tempPlanets[0][i].theta + 1e-3 * dt;
	}
}

void SatelliteEngine::updateAccelerations(uint32_t inputIndex) {
	for (uint32_t i = 0; i < MAX_PLANET_ARRAY_SIZE; i++) {
		tempAccelerations[i] = glm::dvec3(0);//initialise element to 0;
		for (uint32_t j = 0; j < MAX_PLANET_ARRAY_SIZE; j++) {
			if (i == j) {
				continue;
			}
			else {
				glm::dvec3 sep = tempPlanets[inputIndex][j].pos_2 - tempPlanets[inputIndex][i].pos_2;//use pos 2 as they are the most up-to-date positions;
				double aMult = tempPlanets[inputIndex][j].mass * CONSTANT_G/ glm::dot(sep, sep);
				tempAccelerations[i] += aMult * glm::normalize(sep);
			}
		}
	}
}


void SatelliteEngine::cleanup() {
	delete satUBO;

	vkUnmapMemory(device, uniformMemory.memory);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkDestroyBuffer(device, lineBuffer, nullptr);
	for (uint32_t i = 0; i < satBuffers.size(); i++) { vkDestroyBuffer(device, satBuffers[i], nullptr); }
	vkDestroyBuffer(device, lineInfoBuffer, nullptr);
	vkDestroyBuffer(device, fieldMeshBuffer, nullptr);


	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipeline(device, linePipeline, nullptr);
	vkDestroyPipeline(device, meshPipeline, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}