
#include <cstdlib>
#include "grav.h"

void GravEngine::initGrav_A(GravInit details) {
	device = details.device;
	commandPool = details.commandPool;
	descriptorPool = details.descriptorPool;
	gravQueue = details.gravQueue;
	memProperties = details.memProperties;

	shaderCode = details.shaderCode;
	particles = details.particles;

	createStorageBuffers();

	
}
void GravEngine::initGrav_B() {
	createDescriptorSets();
	createPipeline();

	createCommandBuffers();
	createSyncObjects();
}

void GravEngine::createPipeline() {
	//start pipeline creation with shader module

	VkShaderModuleCreateInfo shaderInfo{};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = shaderCode[0]->size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode[0]->data());

	VkShaderModule shader;
	if (vkCreateShaderModule(device, &shaderInfo, nullptr, &shader) != VK_SUCCESS) { throw std::runtime_error("Grav Shader creation failed!"); }

	VkPipelineShaderStageCreateInfo shaderStage{};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = shader;
	shaderStage.pName = "main";

	VkPushConstantRange gravPushRange{};
	gravPushRange.size = sizeof(GravPushConstants);
	gravPushRange.offset = 0;
	gravPushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &gravDescriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &gravPushRange;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &gravPipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravPipelineLayout"); }

	VkComputePipelineCreateInfo pipeInfo{};
	pipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeInfo.layout = gravPipelineLayout;
	pipeInfo.flags = 0;
	pipeInfo.stage = shaderStage;
	pipeInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &gravPipeline) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravPipeline"); }
	
	vkDestroyShaderModule(device, shader, nullptr);

	//now create new shaders sort pipelines;


	VkPipelineLayoutCreateInfo layoutInfo1{};
	layoutInfo1.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo1.setLayoutCount = 1;
	layoutInfo1.pSetLayouts = &sortDescriptorSetLayout;
	layoutInfo1.pushConstantRangeCount = 0;
	layoutInfo1.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &layoutInfo1, nullptr, &sortPipelineLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravPipelineLayout"); }

	for (uint32_t i = 0; i < 3; i++) {
		VkShaderModuleCreateInfo shaderInfo1{};
		shaderInfo1.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo1.codeSize = shaderCode[i+1]->size();
		shaderInfo1.pCode = reinterpret_cast<const uint32_t*>(shaderCode[i+1]->data());

		VkShaderModule shader1;
		if (vkCreateShaderModule(device, &shaderInfo1, nullptr, &shader1) != VK_SUCCESS) { throw std::runtime_error("Grav Shader creation failed!"); }
	
		VkPipelineShaderStageCreateInfo shaderStage1{};
		shaderStage1.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage1.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage1.module = shader1;
		shaderStage1.pName = "main";

		VkComputePipelineCreateInfo pipeInfo1{};
		pipeInfo1.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeInfo1.layout = sortPipelineLayout;
		pipeInfo1.flags = 0;
		pipeInfo1.stage = shaderStage1;
		pipeInfo1.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeInfo1, nullptr, &sortPipelines[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravPipeline"); }

		vkDestroyShaderModule(device, shader1, nullptr);
	}

}

void GravEngine::createDescriptorSets() {
	//first create descriptr set bindings and layout
	std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].pImmutableSamplers = nullptr;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[1].binding = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};

	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &gravDescriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravDescriptorSetLayout"); }

	//then create descriptor sets
	std::array<VkDescriptorSetLayout, COMPUTE_STEPS> layouts{ gravDescriptorSetLayout , gravDescriptorSetLayout, gravDescriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(COMPUTE_STEPS);
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, gravDescriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate GravDescriptorSets"); }

	//then update them with relevant accesses to the storage buffers
	for (size_t i = 0; i < COMPUTE_STEPS; i++) {
		uint32_t readIndex = i;
		uint32_t writeIndex = (i + 1) % COMPUTE_STEPS;

		uint32_t range = static_cast<uint32_t>(particles->size()) * sizeof(Particle);

		uint32_t readOffset = readIndex * range;
		uint32_t writeOffset = writeIndex * range;

		VkDescriptorBufferInfo readBuffer{};
		readBuffer.buffer = storageBuffer;
		readBuffer.offset = readOffset;
		readBuffer.range = range;

		VkDescriptorBufferInfo writeBuffer{};
		writeBuffer.buffer = storageBuffer;
		writeBuffer.offset = writeOffset;
		writeBuffer.range = range;

		std::array<VkWriteDescriptorSet, 2> writes{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = gravDescriptorSets[i];
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[0].pBufferInfo = &readBuffer;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = gravDescriptorSets[i];
		writes[1].dstBinding = 1;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[1].pBufferInfo = &writeBuffer;

		vkUpdateDescriptorSets(device, 2, writes.data(), 0, nullptr);
	}
	createSortDescriptorSets();
}

void GravEngine::createSortDescriptorSets() {
	std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].pImmutableSamplers = nullptr;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[1].binding = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[2].binding = 2;
	bindings[2].descriptorCount = 1;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[2].pImmutableSamplers = nullptr;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[3].binding = 3;
	bindings[3].descriptorCount = 1;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[3].pImmutableSamplers = nullptr;
	bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};

	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &sortDescriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravDescriptorSetLayout"); }

	std::array<VkDescriptorSetLayout, COMPUTE_STEPS> layouts{ sortDescriptorSetLayout , sortDescriptorSetLayout, sortDescriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(COMPUTE_STEPS);
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, sortDescriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate GravDescriptorSets"); }

	for (size_t i = 0; i < COMPUTE_STEPS; i++) {
		VkWriteDescriptorSet baseWrite{};
		baseWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		baseWrite.dstSet = sortDescriptorSets[i];
		baseWrite.dstArrayElement = 0;
		baseWrite.descriptorCount = 1;
		baseWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		std::array<VkWriteDescriptorSet, 4> writes = { baseWrite , baseWrite , baseWrite , baseWrite };

		uint32_t deltaRange = GRID_CELL_COUNT * sizeof(uint32_t) / 2;
		uint32_t offsetRange = GRID_CELL_COUNT * sizeof(uint32_t);
		uint32_t scanRange = GRID_CELL_COUNT * sizeof(uint32_t) / 1024;

		VkDescriptorBufferInfo deltaInfo{};
		deltaInfo.buffer = deltaBuffer;
		deltaInfo.offset = i * deltaRange;
		deltaInfo.range = deltaRange;

		VkDescriptorBufferInfo oldOffsetInfo{};
		oldOffsetInfo.buffer = offsetBuffer;
		oldOffsetInfo.offset = i * offsetRange;
		oldOffsetInfo.range = offsetRange;

		VkDescriptorBufferInfo newOffsetInfo{};
		newOffsetInfo.buffer = offsetBuffer;
		newOffsetInfo.offset = ((i + 1) % COMPUTE_STEPS) * offsetRange;
		newOffsetInfo.range = offsetRange;

		VkDescriptorBufferInfo scanInfo{};
		scanInfo.buffer = scanBuffer;
		scanInfo.offset = i * scanRange;
		scanInfo.range = scanRange;

		//shaders dictate binding
		//0: scan
		//1: newOffsets
		//2: oldOffsets
		//3: deltas


		writes[0].dstBinding = 0;
		writes[0].pBufferInfo = &scanInfo;
		writes[1].dstBinding = 1;
		writes[1].pBufferInfo = &newOffsetInfo;
		writes[2].dstBinding = 2;
		writes[2].pBufferInfo = &oldOffsetInfo;
		writes[3].dstBinding = 3;
		writes[3].pBufferInfo = &deltaInfo;
	
		vkUpdateDescriptorSets(device, 4, writes.data(), 0, nullptr);
	}

}

void GravEngine::createCommandBuffers() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = COMPUTE_STEPS;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(device, &allocInfo, gravCommandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate GravCommandBuffers"); }

	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &transferCommandBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate GravTransferCommandBuffers"); }
}

void GravEngine::createStorageBuffers() {
	uint32_t dataSize = particles->size() * sizeof((*particles)[0]);
	uint32_t bufferSize = dataSize * COMPUTE_STEPS;

	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = bufferSize;

	if (vkCreateBuffer(device, &createInfo, nullptr, &storageBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravStorageBuffer"); }

	vkGetBufferMemoryRequirements(device, storageBuffer, &(storageRequirements.requirements));
	storageRequirements.flags = 0;
	
	createInfo.size = GRID_CELL_COUNT * sizeof(uint32_t) * COMPUTE_STEPS / 2;
	if (vkCreateBuffer(device, &createInfo, nullptr, &deltaBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravStorageBuffer"); }
	vkGetBufferMemoryRequirements(device, deltaBuffer, &(deltaRequirements.requirements));
	createInfo.size = GRID_CELL_COUNT * sizeof(uint32_t) * COMPUTE_STEPS;
	if (vkCreateBuffer(device, &createInfo, nullptr, &offsetBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravStorageBuffer"); }
	vkGetBufferMemoryRequirements(device, offsetBuffer, &(offsetRequirements.requirements));
	createInfo.size = GRID_CELL_COUNT * sizeof(uint32_t) * COMPUTE_STEPS / 1024;
	if (vkCreateBuffer(device, &createInfo, nullptr, &scanBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create GravStorageBuffer"); }
	vkGetBufferMemoryRequirements(device, scanBuffer, &(scanRequirements.requirements));

	scanRequirements.flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	offsetRequirements.flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;;
	deltaRequirements.flags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	scanRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	offsetRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	deltaRequirements.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}

void GravEngine::initMemory(std::array<MemInit, 4> details) {
	storageMemory = details[0].memory;
	storageMemOffset = details[0].offset;
	if (storageRequirements.requirements.size != details[0].range) { throw std::runtime_error("Mismatch in GravStorageMemory size"); }

	std::cout << details[0].range << " | " << storageRequirements.requirements.size << std::endl;

	vkBindBufferMemory(device, storageBuffer, storageMemory, storageMemOffset);

	deltaMem = details[1];
	offsetMem = details[2];
	scanMem = details[3];

	vkBindBufferMemory(device, deltaBuffer, deltaMem.memory, deltaMem.offset);
	vkBindBufferMemory(device, offsetBuffer, offsetMem.memory, offsetMem.offset);
	vkBindBufferMemory(device, scanBuffer, scanMem.memory, scanMem.offset);
}

void GravEngine::syncBufferData_A(MemoryDetails* stagingRequirements) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = sizeof(Particle) * particles->size();
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &createInfo, nullptr, &stagingBuffer) != VK_SUCCESS) { throw std::runtime_error("Failed to create gravEngine staging buffer"); }

	vkGetBufferMemoryRequirements(device, stagingBuffer, &(stagingRequirements->requirements));

	stagingRequirements->flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}
void GravEngine::syncBufferData_B(bool direction, MemInit memory) {
	//direction: true => CPU -> GPU, false => GPU -> CPU

	
	//create staging buffer and allocate memory


	vkBindBufferMemory(device, stagingBuffer, memory.memory, memory.offset);

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

	vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

	uint32_t dataSize = sizeof(Particle) * particles->size();

	VkBufferCopy copyInfo{};
	copyInfo.size = dataSize;
	copyInfo.srcOffset = 0;
	copyInfo.dstOffset = 0;

	if (direction) {
		//load data CPU -> GPU
		for (uint32_t i = 0; i < COMPUTE_STEPS; i++) {
			copyInfo.dstOffset = i * sizeof(Particle) * particles->size();
			vkCmdCopyBuffer(transferCommandBuffer, stagingBuffer, storageBuffer, 1, &copyInfo);
		}
	}
	else {
		//retrieve data GPU -> CPU
		vkCmdCopyBuffer(transferCommandBuffer, storageBuffer, stagingBuffer, 1, &copyInfo);
	}
	
	vkEndCommandBuffer(transferCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCommandBuffer; 

	void* data;

	if (direction) { // if load then prepare data in staging buffer
		vkMapMemory(device, memory.memory, memory.offset, dataSize, 0, &data);
		memcpy(data, particles->data(), dataSize);
		vkUnmapMemory(device, memory.memory);
	}

	vkQueueSubmit(gravQueue, 1, &submitInfo, transferFence);

	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &transferFence);

	if (!direction) { // if retreive then get data from buffer
		vkMapMemory(device, memory.memory, memory.offset, dataSize, 0, &data);
		memcpy(particles->data(), data, dataSize);
		vkUnmapMemory(device, memory.memory);
	}

	//cleanup resources
	//vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkDestroyFence(device, transferFence, nullptr);
	vkResetCommandBuffer(transferCommandBuffer, 0);
}
void GravEngine::simGrav(double dt) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;

	ComputeConstants constants{};
	constants.deltaTime = dt;

	VkCommandBufferSubmitInfo commandInfo{};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

	VkSemaphoreSubmitInfo waitInfo1{};
	waitInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitInfo1.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	waitInfo1.semaphore = gravFinishedSemaphores[computeIndex];

	VkSemaphoreSubmitInfo waitInfo2{};
	waitInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitInfo2.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	waitInfo2.semaphore = renderGravSemaphores[computeIndex];
	
	VkSemaphoreSubmitInfo signalInfo1{};
	signalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalInfo1.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	signalInfo1.semaphore = gravFinishedSemaphores[(computeIndex + 1) % COMPUTE_STEPS];

	VkSemaphoreSubmitInfo signalInfo2{};
	signalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalInfo2.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	signalInfo2.semaphore = gravRenderSemaphores[(computeIndex + 1) % COMPUTE_STEPS];

	std::array<VkSemaphoreSubmitInfo, 2> waitInfos = { waitInfo1 , waitInfo2 };
	std::array<VkSemaphoreSubmitInfo, 2> signalInfos = { signalInfo1 , signalInfo2 };

	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &commandInfo;
	submitInfo.waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size());
	submitInfo.pWaitSemaphoreInfos = waitInfos.data();
	submitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
	submitInfo.pSignalSemaphoreInfos = signalInfos.data();

	if (firstFrame) {
		submitInfo.waitSemaphoreInfoCount = 0;
		submitInfo.pWaitSemaphoreInfos = nullptr;
		firstFrame = false;
	}

	vkWaitForFences(device, 1, &gravFinishedFences[computeIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &gravFinishedFences[computeIndex]);
	vkResetCommandBuffer(gravCommandBuffers[computeIndex], 0);

	if (vkBeginCommandBuffer(gravCommandBuffers[computeIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin simGrav"); }

	vkCmdBindPipeline(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, gravPipeline);
	vkCmdBindDescriptorSets(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, gravPipelineLayout, 0, 1, &gravDescriptorSets[computeIndex], 0, nullptr);

	vkCmdPushConstants(gravCommandBuffers[computeIndex], gravPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeConstants), &constants);

	vkCmdDispatch(gravCommandBuffers[computeIndex], particles->size(), 1, 1);

	if (vkEndCommandBuffer(gravCommandBuffers[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record simGrav"); }

	commandInfo.commandBuffer = gravCommandBuffers[computeIndex];

	if (vkQueueSubmit2(gravQueue, 1, &submitInfo, gravFinishedFences[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to submit simGrav"); }

	computeIndex = (computeIndex + 1) % COMPUTE_STEPS;

	
}

std::array<VkSemaphore, 3> GravEngine::getInterleavedSemaphores(std::array < VkSemaphore, COMPUTE_STEPS> inputs) {
	gravRenderSemaphores = inputs;
	return renderGravSemaphores;
}
VkBuffer GravEngine::getInterleavedStorageBuffer() {
	return storageBuffer;
}

void GravEngine::getMemoryRequirements(std::vector<MemoryDetails>* mem, std::vector<uint16_t>* count) {
	mem->push_back(storageRequirements);
	mem->push_back(deltaRequirements);
	mem->push_back(offsetRequirements);
	mem->push_back(scanRequirements);
	count->push_back(4);
}

void GravEngine::createSyncObjects() {
	VkSemaphoreCreateInfo semInfo{};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	

	VkFenceCreateInfo fenInfo{};
	fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


	for (size_t i = 0; i < COMPUTE_STEPS; i++) {
		if (
			vkCreateSemaphore(device, &semInfo, nullptr, &gravFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semInfo, nullptr, &renderGravSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenInfo, nullptr, &gravFinishedFences[i]) != VK_SUCCESS
			) {
			throw std::runtime_error("Failed to create engine Sync Objects");
		}
	}
}

void GravEngine::cleanup() {
	for (size_t i = 0; i < COMPUTE_STEPS; i++) {
		vkDestroySemaphore(device, gravFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, renderGravSemaphores[i], nullptr);
		vkDestroyFence(device, gravFinishedFences[i], nullptr);
	}
	vkFreeCommandBuffers(device, commandPool, gravCommandBuffers.size(), gravCommandBuffers.data());
	vkFreeDescriptorSets(device, descriptorPool, gravDescriptorSets.size(), gravDescriptorSets.data());

	vkDestroyBuffer(device, storageBuffer, nullptr);
	vkDestroyPipeline(device, gravPipeline, nullptr);
	vkDestroyPipelineLayout(device, gravPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, gravDescriptorSetLayout, nullptr);
}

void GravEngine::createRandomData() {
	offsets.resize(GRID_CELL_COUNT);
	deltaOffsets.resize(GRID_CELL_COUNT/2);
	newOffsets.resize(GRID_CELL_COUNT * 3);

	for (uint32_t i = 0; i < GRID_CELL_COUNT/2; i++) {
		uint32_t value = rand() % 10;
		uint32_t add = 0;
		uint32_t sub = 0;
		add += 3;
		add += 5 << 8;
		add += 4 << 16;
		add += 2 << 24;
		offsets[2*i] = (i ==0) ? value : offsets[2*i - 1] + value;
		offsets[2 * i + 1] = offsets[2 * i] + value + 1;
		deltaOffsets[i] = (i<10)? 0: add;
	}
	void* data;

	//vkMapMemory(device, offsetMem.memory, offsetMem.offset, offsetMem.range, 0, &data);

	//memcpy(data, offsets.data(), offsets.size() * sizeof(uint32_t));

	//vkUnmapMemory(device, offsetMem.memory);

	//vkMapMemory(device, deltaMem.memory, deltaMem.offset, deltaMem.range, 0, &data);

	//memcpy(data, deltaOffsets.data(), deltaOffsets.size() * sizeof(uint32_t));

	//vkUnmapMemory(device,deltaMem.memory);

	runCommands();

	vkMapMemory(device, offsetMem.memory, offsetMem.offset, offsetMem.range, 0, &data);

	memcpy(newOffsets.data(), data, offsets.size() * sizeof(uint32_t));

	vkUnmapMemory(device, offsetMem.memory);



}
void GravEngine::runCommands() {
	VkFence transferFence;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;

	if (vkCreateFence(device, &fenceInfo, nullptr, &transferFence) != VK_SUCCESS) { throw std::runtime_error("Failed to create transfer fence for GravDataSync"); }





	std::cout << 0 << std::endl;
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	vkResetCommandBuffer(gravCommandBuffers[computeIndex], 0);
	if (vkBeginCommandBuffer(gravCommandBuffers[computeIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin simGrav"); }

	vkCmdBindPipeline(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, sortPipelines[0]);
	vkCmdBindDescriptorSets(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, sortPipelineLayout, 0, 1, &sortDescriptorSets[computeIndex], 0, nullptr);


	vkCmdDispatch(gravCommandBuffers[computeIndex], GRID_CELL_COUNT / 1024, 1, 1);


	if (vkEndCommandBuffer(gravCommandBuffers[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record simGrav"); }

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &gravCommandBuffers[computeIndex];
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.waitSemaphoreCount = 0;
	auto result = vkQueueSubmit(gravQueue, 1, &submitInfo, transferFence);
	std::cout << 1 << std::endl;
	auto result2 = vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	std::cout << result << std::endl;
	std::cout << result2 << std::endl;
	vkResetFences(device, 1, &transferFence);
	

	vkResetCommandBuffer(gravCommandBuffers[computeIndex], 0);
	if (vkBeginCommandBuffer(gravCommandBuffers[computeIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin simGrav"); }

	vkCmdBindPipeline(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, sortPipelines[1]);
	vkCmdBindDescriptorSets(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, sortPipelineLayout, 0, 1, &sortDescriptorSets[computeIndex], 0, nullptr);

	vkCmdDispatch(gravCommandBuffers[computeIndex], 1, 1, 1);

	if (vkEndCommandBuffer(gravCommandBuffers[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record simGrav"); }


	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &gravCommandBuffers[computeIndex];
	vkQueueSubmit(gravQueue, 1, &submitInfo, transferFence);
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &transferFence);

	std::cout << 2 << std::endl;
	vkResetCommandBuffer(gravCommandBuffers[computeIndex], 0);
	if (vkBeginCommandBuffer(gravCommandBuffers[computeIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to begin simGrav"); }
	vkCmdBindPipeline(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, sortPipelines[2]);

	vkCmdBindDescriptorSets(gravCommandBuffers[computeIndex], VK_PIPELINE_BIND_POINT_COMPUTE, sortPipelineLayout, 0, 1, &sortDescriptorSets[computeIndex], 0, nullptr);

	vkCmdDispatch(gravCommandBuffers[computeIndex], GRID_CELL_COUNT / 1024, 1, 1);
	if (vkEndCommandBuffer(gravCommandBuffers[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record simGrav"); }


	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &gravCommandBuffers[computeIndex];
	vkQueueSubmit(gravQueue, 1, &submitInfo, transferFence);
	vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &transferFence);
	vkDestroyFence(device, transferFence, nullptr);
	std::cout << 3 << std::endl;





}