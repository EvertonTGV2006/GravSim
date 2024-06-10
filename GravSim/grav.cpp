

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
	shaderInfo.codeSize = shaderCode->size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode->data());

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

}

void GravEngine::initMemory(MemInit details) {
	storageMemory = details.memory;
	storageMemOffset = details.offset;
	if (storageRequirements.requirements.size != details.range) { throw std::runtime_error("Mismatch in GravStorageMemory size"); }

	std::cout << details.range << " | " << storageRequirements.requirements.size << std::endl;

	vkBindBufferMemory(device, storageBuffer, storageMemory, storageMemOffset);
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
	count->push_back(1);
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