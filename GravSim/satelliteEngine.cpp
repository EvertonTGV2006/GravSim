#include "satelliteEngine.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>


void SatelliteEngine::initSatEngine_A(SatInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;

	memProperties = details.memProperties;

	planets = details.planets;

	params = details.params;

	memcpy(shaderCode.data(), details.shaderCode.data(), shaderCode.size() * sizeof(shaderCode[0]));

	createBuffers();

	satUBO = new SatPlanetBuffer;
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
	
	bufferInfo.size = FRAMES_IN_FLIGHT * sizeof(SatPlanetBuffer);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	planetSize = bufferInfo.size / FRAMES_IN_FLIGHT;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &planetHostBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create Satellite uniform buffer"); }
	vkGetBufferMemoryRequirements(device, planetHostBuffer, &planetHostRequirements.requirements);

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

	bufferInfo.size = sizeof(SatPlanetBuffer);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &planetBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create field mesh buffer"); }
	vkGetBufferMemoryRequirements(device, planetBuffer, &planetRequirements.requirements);
	

	bufferInfo.size = SATELLITE_COUNT * sizeof(SatInfo);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &satInfoBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create field mesh buffer"); }
	vkGetBufferMemoryRequirements(device, satInfoBuffer, &satInfoRequirements.requirements);
	satInfoSize = bufferInfo.size;


	bufferInfo.size = SATELLITE_COUNT * sizeof(Satellite);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &satTransferBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create field mesh buffer"); }
	vkGetBufferMemoryRequirements(device, satTransferBuffer, &satTransferRequirements.requirements);
	satTransferSize = bufferInfo.size;

	//so if we have a mesh with MESH_STEPS_MAJOR * MESH_STEPS_MAJOR
	//we have rectangle grid MESH_STEPS_MAJOR - 1 ** 2
	//so that * 2 tri
	// * 3 indices
	// * sizeof(index) == sizeof(uint16_t)
	bufferInfo.size = (fieldMeshResMajor - 1) * (fieldMeshResMajor - 1) * 2 * 3 * sizeof(uint32_t);
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &fieldMeshIndexBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create field mesh buffer"); }
	vkGetBufferMemoryRequirements(device, fieldMeshIndexBuffer, &fieldMeshIndexRequirements.requirements);



	lineRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	satRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	planetHostRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	lineInfoRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	fieldMeshRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	planetRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	satInfoRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	satTransferRequirements.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	fieldMeshIndexRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

void SatelliteEngine::createDescriptorSets() {
	VkDescriptorSetLayoutBinding ubo{};
	ubo.binding = 0;
	ubo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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

	VkDescriptorSetLayoutBinding sIBuffer{};
	sIBuffer.binding = 6;
	sIBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	sIBuffer.descriptorCount = 1;
	sIBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	sIBuffer.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 7> bindings = { ubo, iBuffer, oBuffer,lBuffer,lIBuffer,mBuffer ,sIBuffer};

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
		bufferInfo.buffer = planetBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = planetSize;

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

		VkDescriptorBufferInfo sIInfo{};
		sIInfo.buffer = satInfoBuffer;
		sIInfo.offset = 0;
		sIInfo.range = satInfoSize;

		std::array<VkWriteDescriptorSet, 7> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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

		descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[6].dstSet = descriptorSets[i];
		descriptorWrites[6].dstBinding = 6;
		descriptorWrites[6].dstArrayElement = 0;
		descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[6].descriptorCount = 1;
		descriptorWrites[6].pBufferInfo = &sIInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);


		iInfo.buffer = satBuffers[1];
		oInfo.buffer = satBuffers[0];


		descriptorWrites[0].dstSet = descriptorSets[i + 1];
		descriptorWrites[1].dstSet = descriptorSets[i + 1];
		descriptorWrites[2].dstSet = descriptorSets[i + 1];
		descriptorWrites[3].dstSet = descriptorSets[i + 1];
		descriptorWrites[4].dstSet = descriptorSets[i + 1];
		descriptorWrites[5].dstSet = descriptorSets[i + 1];
		descriptorWrites[6].dstSet = descriptorSets[i + 1];

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
	specConstantsData.STEPS_PER_SHADER = STEPS_PER_SHADER;
	specConstantsData.MESH_PER_SHADER = MESH_PER_SHADER;

	std::array<VkSpecializationMapEntry, 10> specEntries{};
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
	specEntries[8].constantID = 8;
	specEntries[8].offset = offsetof(SatSpecConstants, STEPS_PER_SHADER);
	specEntries[8].size = sizeof(specConstantsData.STEPS_PER_SHADER);
	specEntries[9].constantID = 9;
	specEntries[9].offset = offsetof(SatSpecConstants, MESH_PER_SHADER);
	specEntries[9].size = sizeof(specConstantsData.MESH_PER_SHADER);

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
	mem->push_back(planetHostRequirements);
	mem->push_back(lineInfoRequirements);
	mem->push_back(fieldMeshRequirements);
	mem->push_back(planetRequirements);
	mem->push_back(satInfoRequirements);
	mem->push_back(satTransferRequirements);
	mem->push_back(fieldMeshIndexRequirements);
	count->push_back(9);
}
void SatelliteEngine::initMemory(MemInit* detPtr) {
	std::array<MemInit, 9> details{};
	memcpy(details.data(), detPtr, details.size() * sizeof(MemInit));

	lineMemory = details[0];
	satMemory = details[1];
	planetHostMemory = details[2];
	lineInfoMemory = details[3];
	fieldMeshMemory = details[4];
	planetMemory = details[5];
	satInfoMemory = details[6];
	satTransferMemory = details[7];
	fieldMeshIndexMemory = details[8];

	vkBindBufferMemory(device, lineBuffer, lineMemory.memory, lineMemory.offset);

	for (uint32_t i = 0; i < satBuffers.size(); i++) { vkBindBufferMemory(device, satBuffers[i], satMemory.memory, satMemory.offset + i * satSize); }

	vkBindBufferMemory(device, planetHostBuffer, planetHostMemory.memory, planetHostMemory.offset);
	
	void* data;

	vkMapMemory(device, planetHostMemory.memory, planetHostMemory.offset, planetHostMemory.range, 0, &data);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		planetBuffersMapped[i] = reinterpret_cast<char*>(data) + i * planetSize;
	}

	vkBindBufferMemory(device, lineInfoBuffer, lineInfoMemory.memory, lineInfoMemory.offset);

	vkBindBufferMemory(device, fieldMeshBuffer, fieldMeshMemory.memory, fieldMeshMemory.offset);

	vkBindBufferMemory(device, planetBuffer, planetMemory.memory, planetMemory.offset);

	vkBindBufferMemory(device, satInfoBuffer, satInfoMemory.memory, satInfoMemory.offset);

	vkBindBufferMemory(device, satTransferBuffer, satTransferMemory.memory, satTransferMemory.offset);
	
	vkMapMemory(device, satTransferMemory.memory, satTransferMemory.offset, satTransferMemory.range, 0, &data);

	satTransferMapped = reinterpret_cast<char*>(data);

	vkBindBufferMemory(device, fieldMeshIndexBuffer, fieldMeshIndexMemory.memory, fieldMeshIndexMemory.offset);

}
void SatelliteEngine::initBufferData_A(MemoryDetails* stagingRequiements) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = std::max(std::max(satRequirements.requirements.size, satInfoRequirements.requirements.size), fieldMeshIndexRequirements.requirements.size);
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

	Orbit orb{};
	getOrbitalParams(&satData[1], 0, &orb);

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
	vkResetFences(device, 1, &transferFence);
	vkResetCommandBuffer(transferCommandBuffer, 0);

	SatInfo nullInfo{};
	nullInfo.relDistance = 1e15; //big number 

	for (uint32_t i = 0; i < SATELLITE_COUNT; i++) {
		char* cpyPtr = reinterpret_cast<char*>(data) + (i * sizeof(SatInfo));
		memcpy(cpyPtr, &nullInfo, sizeof(SatInfo));
	}

	cpy.size = satInfoSize;

	if (vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin transfer command buffer"); }
	vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, satInfoBuffer, 1, &cpy);
	if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to end transfer command buffer"); }
	if (vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to submit transfer command buffer"); }
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &transferFence);
	vkResetCommandBuffer(transferCommandBuffer, 0);

	//index buffer;
	uint32_t writeIndex = 0;
	const uint32_t totalSize = (fieldMeshResMajor - 1) * (fieldMeshResMajor - 1) * 2 * 3;
	std::array<uint32_t, totalSize>* indexData = new std::array<uint32_t, totalSize>;
	for (uint32_t i = 0; i < fieldMeshResMajor - 1; i++) {
		for (uint32_t j = 0; j < fieldMeshResMajor - 1; j++) {
			/*
			A ----- B
			| \		|
			|	\	|
			|	  \	|
			C ----- D
			
			C = (i, j)
			A = (i + 1, j);
			B = (i + 1, j + 1);
			D = (i, j + 1)
			
			*/
			//DAC
			(*indexData)[writeIndex] = i * fieldMeshResMajor + (j + 1); //D
			writeIndex++;
			(*indexData)[writeIndex] = (i + 1) * fieldMeshResMajor + j; //A
			writeIndex++;
			(*indexData)[writeIndex] = i * fieldMeshResMajor + j; //C
			writeIndex++;

			//DBA

			(*indexData)[writeIndex] = i * fieldMeshResMajor + (j + 1); //D
			writeIndex++;
			(*indexData)[writeIndex] = (i + 1) * fieldMeshResMajor + (j + 1); //B
			writeIndex++;
			(*indexData)[writeIndex] = (i + 1) * fieldMeshResMajor + j; //A
			writeIndex++;
		}
	}
	memcpy(data, indexData->data(), indexData->size() * sizeof(uint32_t));

	cpy.size = indexData->size() * sizeof(uint32_t);

	if (vkBeginCommandBuffer(transferCommandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin transfer command buffer"); }
	vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, fieldMeshIndexBuffer, 1, &cpy);
	if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to end transfer command buffer"); }
	if (vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to submit transfer command buffer"); }
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);

	vkUnmapMemory(device, memory.memory);


	vkDestroyFence(device, transferFence, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);

}

void SatelliteEngine::simulateSats(VkCommandBuffer commandBuffer, uint32_t frameIndex, double dt) {

	dt = 1.0/400;
	dt *= 1e1;

	dt = params->dt;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	
	
	SatPushConstants satPC{};
	satPC.deltaTime = dt;
	satPC.targetPlanet = 1;
	

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

	VkBufferCopy cpy{};
	cpy.dstOffset = 0;
	cpy.size = planetSize;
	cpy.srcOffset = frameIndex * planetSize;
	vkCmdCopyBuffer(commandBuffer, planetHostBuffer, planetBuffer, 1, &cpy);

	for (uint32_t i = 0; i < COMPUTE_STEPS_PER_FRAME; i++) {
		updatePlanets(0.5 * satPC.deltaTime);
		updatePlanets(0.5 * satPC.deltaTime);
		memcpy(satUBO->planetData[i].data(), planets->data(), planets->size() * sizeof(Planet));

		satPC.planetIndex = i;
		satPC.elapsedTime = elapsedTime;
		elapsedTime += dt;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[2 * frameIndex + i % 2], 0, nullptr);

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SatPushConstants), &satPC);

		vkCmdDispatch(commandBuffer, shaderDispatches, 1, 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(barriers.size()), barriers.data(), 0, nullptr);
	}

	memcpy(planetBuffersMapped[frameIndex], satUBO, sizeof(SatPlanetBuffer));

	if (lineFrame == WRITE_FRAME) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, linePipeline);

		satPC.deltaTime = float(lineCursor);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SatPushConstants), &satPC);

		vkCmdDispatch(commandBuffer, shaderDispatches, 1, 1);

		if (params->mesh) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, meshPipeline);
			vkCmdDispatch(commandBuffer, (fieldMeshResMajor * 2)/MESH_PER_SHADER, 1, 1);
		}

		VkMemoryBarrier mem{};
		mem.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		mem.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		mem.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

		VkMemoryBarrier mem2{};
		mem2.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		mem2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		mem2.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

		//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &mem, 0, nullptr, 0, nullptr);
		//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &mem2, 0, nullptr, 0, nullptr);
		//for combined compute and graphics queue, synchronization now implied by semaphores

		lineCursor = (lineCursor + 1) % LINE_VERTEX_COUNT;
		lineSegments = (lineSegments < 1024) ? lineSegments + 1 : 1024;
	}
	lineFrame = (lineFrame + 1) % FRAMES_PER_LINE;

}
void SatelliteEngine::simulateSatsRaw(uint32_t frameIndex, double dt) {

	dt = 1.0 / 400.0;
	dt *= 1e1;




	SatPushConstants satPC{};
	satPC.deltaTime = dt;
	satPC.targetPlanet = 1;


	uint32_t shaderDispatches = uint32_t(ceil((float(SATELLITE_COUNT) / float(SATELLITES_PER_SHADER)) / 1024.0));

	for (uint32_t i = 0; i < COMPUTE_STEPS_PER_FRAME; i++) {
		updatePlanets(0.5 * satPC.deltaTime);
		updatePlanets(0.5 * satPC.deltaTime);
		memcpy(satUBO->planetData[i].data(), planets->data(), planets->size() * sizeof(Planet));

		satPC.planetIndex = i;
		satPC.elapsedTime = elapsedTime;
		elapsedTime += dt;


	}

	memcpy(planetBuffersMapped[frameIndex], satUBO, sizeof(SatPlanetBuffer));


	lineFrame = (lineFrame + 1) % FRAMES_PER_LINE;

}
SatExternalMembers SatelliteEngine::getSatellitePtrs() {
	SatExternalMembers data{};
	data.lineBuffer = &lineBuffer;
	data.lineCursor = &lineCursor;
	data.lineSegments = &lineSegments;
	data.lineInfoBuffer = &lineInfoBuffer;
	data.meshBuffer = &fieldMeshBuffer;
	data.meshIndexBuffer = &fieldMeshIndexBuffer;
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

void SatelliteEngine::getOrbitalParams(Satellite* sat, uint32_t planetIndex, Orbit* result) {
	(*result) = {};
	Planet targetPlanet = (*planets)[planetIndex];
	double mu = CONSTANT_G * targetPlanet.mass;
	glm::dvec3 r = sat->pos - targetPlanet.pos_2;
	glm::dvec3 v = sat->vel - targetPlanet.vel_2;
	glm::dvec3 h = glm::cross(r, v); //angular momentum, perpendicular to orbital plane
	glm::dvec3 e = (glm::cross(v, h) / mu) - glm::normalize(r); //ecentricity vector, points towards perapsis
	glm::dvec3 n = glm::cross(h, glm::dvec3(0, 0, 1)); //points to ascending node, (0, 0, 1) is normal to refernce plance
	if (glm::length(n) != 0) {
		double a = (glm::dot(h, h) / mu) / (1 - glm::dot(e, e)); //semiMajorAxis. p = h^2/mu; p = a(1-e^2) so a = (h^2/mu)/(1-e^2)
		double inclination = glm::acos(glm::dot(glm::normalize(h), glm::dvec3(0, 0, 1))); //compute inclination
		double argAscend = glm::acos(glm::dot(glm::normalize(n), glm::dvec3(1, 0, 0)));
		double argPeri = glm::acos(glm::dot(glm::normalize(n), glm::normalize(e)));
		double trueAnom = glm::acos(glm::dot(glm::normalize(r), glm::normalize(e)));
		result->argPeriapsis = argPeri;
		result->planetIndex = planetIndex;
		result->semiMajorAxis = a;
		result->eccentricity = glm::length(e);
		result->ascNodeLong = argAscend;
		result->inclination = inclination;
		result->trueAnomaly = trueAnom;
	}
	else {
		double a = (glm::dot(h, h) / mu) / (1 - glm::dot(e, e)); //semiMajorAxis. p = h^2/mu; p = a(1-e^2) so a = (h^2/mu)/(1-e^2)
		double inclination = glm::acos(glm::dot(glm::normalize(h), glm::dvec3(0, 0, 1))); //compute inclination
		double argAscend = 0;
		double argPeri = glm::acos(glm::dot(glm::normalize(e), glm::dvec3(1, 0, 0)));
		double trueAnom = glm::acos(glm::dot(glm::normalize(r), glm::normalize(e)));
		result->argPeriapsis = argPeri;
		result->planetIndex = planetIndex;
		result->semiMajorAxis = a;
		result->eccentricity = glm::length(e);
		result->ascNodeLong = argAscend;
		result->inclination = inclination;
		result->trueAnomaly = trueAnom;
	}



	


	//determine orbital paramaters following the procedure on the following page https://en.wikipedia.org/wiki/Orbit_determination#Orbit_Determination_from_a_State_Vector

}

void SatelliteEngine::satelliteTransfer(VkCommandBuffer transferCommandBuffer, VkQueue transferQueue, bool direction) {
	//direction true := CPU -> GPU;
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

	if (direction) {
		memcpy(satTransferMapped, satData.data(), satData.size() * sizeof(Satellite));
		for (uint32_t i = 0; i < satBuffers.size(); i++) {
			vkCmdCopyBuffer(transferCommandBuffer, satTransferBuffer, satBuffers[i], 1, &cpy);
		}
	}
	else {
		vkCmdCopyBuffer(transferCommandBuffer, satBuffers[0], satTransferBuffer, 1, &cpy);
	}


	if (vkEndCommandBuffer(transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to end transfer command buffer"); }
	if (vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to submit transfer command buffer"); }
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	if (!direction) {
		memcpy(satData.data(), satTransferMapped, satData.size() * sizeof(Satellite));
	}


	vkResetFences(device, 1, &transferFence);
	vkResetCommandBuffer(transferCommandBuffer, 0);
	vkDestroyFence(device, transferFence, nullptr);


}

void SatelliteEngine::cleanup() {
	delete satUBO;

	vkUnmapMemory(device, planetHostMemory.memory);
	vkUnmapMemory(device, satTransferMemory.memory);
	vkDestroyBuffer(device, planetHostBuffer, nullptr);
	vkDestroyBuffer(device, lineBuffer, nullptr);
	for (uint32_t i = 0; i < satBuffers.size(); i++) { vkDestroyBuffer(device, satBuffers[i], nullptr); }
	vkDestroyBuffer(device, lineInfoBuffer, nullptr);
	vkDestroyBuffer(device, fieldMeshBuffer, nullptr);
	vkDestroyBuffer(device, planetBuffer, nullptr);
	vkDestroyBuffer(device, satInfoBuffer, nullptr);
	vkDestroyBuffer(device, satTransferBuffer, nullptr);
	vkDestroyBuffer(device, fieldMeshIndexBuffer, nullptr);


	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipeline(device, linePipeline, nullptr);
	vkDestroyPipeline(device, meshPipeline, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}