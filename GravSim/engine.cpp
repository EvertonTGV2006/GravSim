#include "engine.h"

#include <GLFW/glfw3.h>


#include "window.h"
#include "structs.h"
#include "geometry.h"
#include "player.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <optional>
#include <set>
#include <array>
#include <bitset>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLFM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <thread>
#include <chrono>
#include <algorithm>
#include <numeric>

//for chooseSwapExtent();
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include "toml.hpp"




void VulkanEngine::initEngine() {
    stat->addMessage(MSG_LEVEL_STARTUP, "Starting Engine");

    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    winmanager.initWindow();
    player->winmanager = &winmanager;
    player->planets = &planets;
    player->updateGLFWcallbacks();
    player->initUIElements(&frameCounter, &fpsVal);

    uiRasterizer.stat = stat;

    std::vector<Mesh> meshes;
    meshes.resize(1);
    meshes[0].vertices = &vertices;
    meshes[0].indices = &indices;
    SphereGeometry sphere{};
    sphere.vertices = meshes[0].vertices;
    sphere.indices = meshes[0].indices;

    std::thread spheret(&SphereGeometry::createSphereIcosphere, &sphere, 4);

    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorPool();
    createCommandPools();
    createColourResources();
    createDepthResources();
    createFramebuffers();
    createSyncObjects();
    createCommandBuffers();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Created shared engine resources");

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);




    std::vector<std::vector<char>> shaderCode;
    std::vector<std::string> shaderFiles;
    std::vector<uint16_t> shaderCounts;


    shaderCounts.push_back(static_cast<uint16_t>(shaderFiles.size()));
    shaderFiles.insert(std::end(shaderFiles), std::begin(particleRasterizer.shaderFiles), std::end(particleRasterizer.shaderFiles));
    shaderCounts.push_back(static_cast<uint16_t>(shaderFiles.size()));
    shaderFiles.insert(std::end(shaderFiles), std::begin(uiRasterizer.shaderFiles), std::end(uiRasterizer.shaderFiles));
    shaderCounts.push_back(static_cast<uint16_t>(shaderFiles.size()));
    shaderFiles.insert(std::end(shaderFiles), std::begin(satEngine.shaderFiles), std::end(satEngine.shaderFiles));
    shaderCounts.push_back(static_cast<uint16_t>(shaderFiles.size()));
    
    
    uint16_t shaderCursor = 0;

    readFiles(shaderFiles, &shaderCode);


    shaderCursor = 1;
    UIInit ui{};
    ui.descriptorPool = descriptorPool;
    ui.device = device;
    ui.memProperties = memProperties;
    ui.player = player;
    ui.renderPass = renderPass;
    ui.msaaSamples = msaaSamples;
    ui.aspectRatio = &swapChainAspectRatio;
    for (uint16_t i = shaderCounts[shaderCursor]; i < shaderCounts[shaderCursor + 1]; i++) {
        ui.shaderCode.push_back(&shaderCode[i]);
    }

    shaderCursor = 2;
    SatInit sat{};
    sat.descriptorPool = descriptorPool;
    sat.device = device;
    sat.memProperties = memProperties;
    sat.planets = &planets;
    sat.settings = &player->settings;
    for (uint16_t i = shaderCounts[shaderCursor]; i < shaderCounts[shaderCursor + 1]; i++) {
        sat.shaderCode.push_back(&shaderCode[i]);
    }

    std::thread sattA(&SatelliteEngine::initSatEngine_A, &satEngine, sat);


    std::thread uitA(&UIRasterizer::initUI_A, &uiRasterizer, ui);
    //uiRasterizer.initUI_A(ui);



    spheret.join();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialised shader geometry");

    meshes[0].vertexCount = static_cast<uint32_t>(meshes[0].vertices->size());
    meshes[0].indexCount = static_cast<uint32_t>(meshes[0].indices->size());
    //gravt.join();

    shaderCursor = 0;
    RastInit rast{};
    rast.device = device;
    rast.descriptorPool = descriptorPool;
    rast.renderPass = renderPass;
    rast.msaaSamples = msaaSamples;
    rast.memProperties = memProperties;
    rast.meshes = meshes;
    rast.planets = &planets;
    rast.settings = &player->settings;
    for (uint16_t i = shaderCounts[shaderCursor]; i < shaderCounts[shaderCursor + 1]; i++) {
        rast.shaderCode.push_back(&shaderCode[i]);
    }


    std::thread rasttA(&particleRasterizer::initRast_A, &particleRasterizer, rast);
    //particleRasterizer.initRast_A(rast);
    
    rasttA.join();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialized Particle Rasterizer A");
    uitA.join();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialized UI Rasterizer A");
    sattA.join();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialized Satellite Engine A");


   // particleRasterizer.storeGravStorageBuffer(gravEngine.getInterleavedStorageBuffer());

    allocateMemory();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Allocated Memory");

    particleRasterizer.setExternalPtrs(satEngine.getSatellitePtrs());

    uiRasterizer.initUI_B();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialized UI Rasterizer B");
    particleRasterizer.initRast_B();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialized Particle Rasterizer B");
    satEngine.initSatEngine_B();
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Initialized Satellite Engine B");

    //std::thread uitB(&UIRasterizer::initUI_B, &uiRasterizer);
    //std::thread rastB(&particleRasterizer::initRast_B, &particleRasterizer);
    //std::thread gravtB(&GravEngine::initGrav_B, &gravEngine);

    //uitB.join();
    //rastB.join();
    //gravtB.join();

    initSubclassData();

    //gravEngine.createRandomData();

    


    std::chrono::time_point endTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsedTime = endTime - startTime;
    std::string timeStr = /*std::format("{:%S}", elapsedTime)*/std::to_string(elapsedTime.count());

    //std::cout <<std::endl<<"Program took " << elapsedTime << " to start." << std::endl;
    stat->addMessage(MSG_LEVEL_USER, "Program took " + timeStr + " seconds to start");
}

void VulkanEngine::readFiles(std::vector<std::string> files, std::vector<std::vector<char>>* code) {
    code->resize(files.size());
    for (size_t i = 0; i < files.size(); i++) {
        std::ifstream file(files[i], std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file");
        }
        size_t fileSize = (size_t)file.tellg();
        (*code)[i].resize(fileSize);
        file.seekg(0);
        file.read((*code)[i].data(), fileSize);
        file.close();

        /*std::cout << "Loaded " << files[i] << std::endl;*/
        stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Loaded " + files[i]);
    }
}


//runtime functions
void VulkanEngine::startDraw() {
    //std::thread compute(&VulkanEngine::runCompute, this);
    std::thread graphics(&VulkanEngine::runGraphics, this);
    while (!glfwWindowShouldClose(winmanager.window)) {
        glfwPollEvents();
    }
    //compute.join();
    graphics.join();
}
void VulkanEngine::runGraphics() {
    while (!glfwWindowShouldClose(winmanager.window)) {
        executeCompute();
        executeGraphics();
    }
}
void VulkanEngine::executeGraphics() {
    bool commandSubmitFrame = false;
    //check player window should close state
    if (player->windowShouldClose == true) {
        glfwSetWindowShouldClose(winmanager.window, GLFW_TRUE);
    }
    //unlimitedFPS = false;
    //targetFrameTime_uS = 1e6f / 60;
    //now we wait until next frame;
    if (!unlimitedFPS) {
        std::this_thread::sleep_until(nextFrameScheduled);
        nextFrameScheduled += std::chrono::microseconds(targetFrameTime_uS);
    }
    ct = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration<double>(ct - pt);
    pt = ct;

    player->boxes[2].textCount = 0;

    auto waitStart = std::chrono::high_resolution_clock::now();
    vkWaitForFences(device, 1, &gfFences[frameIndex], VK_TRUE, UINT64_MAX);
    auto waitDuration = std::chrono::high_resolution_clock::now() - waitStart;
    //std::cout << "Waited FPS " << 1e9 / waitDuration.count() << "\n";
    vkResetFences(device, 1, &gfFences[frameIndex]);
    vkResetCommandBuffer(gCommandBuffers[frameIndex], 0);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, igSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    if (vkBeginCommandBuffer(gCommandBuffers[frameIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to start draw recording"); }


    //satEngine.simulateSats(gCommandBuffers[frameIndex], frameIndex, (firstFrame) ? 0.00001f : (float)dt.count());

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    std::array<VkClearValue, 2> clearValues{};
    //clearValues[0].color = { {0.2f, 0.3f, 1.0f, 1.0f} };
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(gCommandBuffers[frameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(gCommandBuffers[frameIndex], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(gCommandBuffers[frameIndex], 0, 1, &scissor);

    //CameraPushConstants camera{};
    //camera.cameraPos = glm::vec4(player->pos, 1.0);
    //camera.viewDirection = glm::vec4(player->viewDirection, 1.0f);
    //camera.lightColour = { 1.0f, 1.0f, 1.0f, 0.0f };
    //camera.lightPos = { 2.0f, 0.0f, 3.0f , 0.0f };

    player->updatePlayerMovement();
    player->updateViewMat();
    UniformBufferObject ubo{};

    ubo.view = player->viewMat;
    float nearPlane = 1e6;
    float farPlane = 1e10;

    ubo.proj = glm::perspective(glm::radians(75.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, /*0.1f*/nearPlane, /*1000.0f*/farPlane);
    ubo.proj[1][1] *= -1;


    
    particleRasterizer.drawObjects(gCommandBuffers[frameIndex], frameIndex, ubo, (firstFrame) ? 0.00001f : (float)dt.count());


    uiRasterizer.drawElements(gCommandBuffers[frameIndex], frameIndex);







    vkCmdEndRenderPass(gCommandBuffers[frameIndex]);

    if (vkEndCommandBuffer(gCommandBuffers[frameIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record draw"); }

    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = gCommandBuffers[frameIndex];

    

    VkSemaphoreSubmitInfo waitInfo1{};
    waitInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo1.semaphore = igSemaphores[frameIndex];
    waitInfo1.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo waitInfo2{};
    waitInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo2.semaphore = cgSemaphores[frameIndex];
    waitInfo2.stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    VkSemaphoreSubmitInfo signalInfo1{};
    signalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo1.semaphore = gpSemaphores[frameIndex];
    signalInfo1.stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSemaphoreSubmitInfo signalInfo2{};
    signalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo2.semaphore = gcSemaphores[frameIndex];
    signalInfo2.stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    std::array<VkSemaphoreSubmitInfo, 2> waitInfos = { waitInfo1, waitInfo2}; //cg, ia
    std::array<VkSemaphoreSubmitInfo, 2> signalInfos = { signalInfo1, signalInfo2 }; //rf, gc

    //for compute
    //wait: cc[-1], gc //cc ensures no compute overlap //gc ensures no compute runaway /desync with graphics.
    //signal cc[0], cg  

    VkSubmitInfo2 submitInfo2{};
    submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo2.commandBufferInfoCount = 1;
    submitInfo2.pCommandBufferInfos = &commandBufferInfo;
    submitInfo2.waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size());
    submitInfo2.pWaitSemaphoreInfos = waitInfos.data();
    submitInfo2.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
    submitInfo2.pSignalSemaphoreInfos = signalInfos.data();
    //vkQueueSubmit2(graphicsQueue, 1, &submitInfo, nullptr);

    //if (firstFrame) {
    //    submitInfo2.waitSemaphoreInfoCount = 1;
    //    submitInfo2.pWaitSemaphoreInfos = &waitInfo1;
    //    firstFrame = false;
    //}


    if (vkQueueSubmit2(graphicsQueue, 1, &submitInfo2, gfFences[frameIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to submit draw command buffer"); }


    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSemaphore waitSemaphores[] = { gpSemaphores[frameIndex] };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;
    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    result = vkQueuePresentKHR(presentQueue, &presentInfo);


    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || player->framebufferResized) {
        recreateSwapChain();
        player->framebufferResized = false;
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }


    //gravEngine.simGrav(dt.count());


    //if (frameTimes.size() % 200 == 0) {
    //    std::cout << "Player Pos: ";
    //    glm::vec3 v = player->pos;
    //    std::cout << v.x << ", " << v.y << ", " << v.z;
    //    std::cout << std::endl;
    //}


    //gravEngine.createRandomData();

    //std::cout << "FPS: " << 1.0 / dt.count() << std::endl;

    frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
    frameCounter++;

    fpsAverage[fpsIndex] = dt.count();
    fpsIndex = (fpsIndex + 1) % 10;

    double fpsSum = 0;
    for (uint32_t i = 0; i < fpsAverage.size(); i++) {
        fpsSum += fpsAverage[i];
    }
    fpsVal = uint32_t(10 / fpsSum);



    //check if validate particles
}
void VulkanEngine::runCompute() {
    while (!glfwWindowShouldClose(winmanager.window)) {
        executeCompute();
    }
}
void VulkanEngine::executeCompute() {
    vkWaitForFences(device, 1, &cfFences[computeIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &cfFences[computeIndex]);
    bool oneTimeRecord = true;
    if (oneTimeRecord){
        if (firstComputeCycle) {
            vkResetCommandBuffer(cCommandBuffers[computeIndex], 0);
        }

        if (!firstComputeCycle) {
            satEngine.simulateSatsRaw(computeIndex, 0.01);
        }
        else {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            if (vkBeginCommandBuffer(cCommandBuffers[computeIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to start draw recording"); }

            satEngine.simulateSats(cCommandBuffers[computeIndex], computeIndex, 0.01);

            if (vkEndCommandBuffer(cCommandBuffers[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record draw"); }
        }
    }
    else {
        vkResetCommandBuffer(cCommandBuffers[computeIndex], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        if (vkBeginCommandBuffer(cCommandBuffers[computeIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to start draw recording"); }

        satEngine.simulateSats(cCommandBuffers[computeIndex], computeIndex, 0.01);

        if (vkEndCommandBuffer(cCommandBuffers[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record draw"); }
    }

    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = cCommandBuffers[computeIndex];


    uint32_t prevIndex = (computeIndex + (FRAMES_IN_FLIGHT - 1)) % FRAMES_IN_FLIGHT;
    VkSemaphoreSubmitInfo waitInfo1{};
    waitInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo1.semaphore = ccSemaphores[prevIndex];//no wait on first frame
    waitInfo1.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    VkSemaphoreSubmitInfo waitInfo2{};
    waitInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo2.semaphore = gcSemaphores[computeIndex]; //no wait on first frame
    waitInfo2.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    VkSemaphoreSubmitInfo signalInfo1{};
    signalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo1.semaphore = ccSemaphores[computeIndex];
    signalInfo1.stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSemaphoreSubmitInfo signalInfo2{};
    signalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo2.semaphore = cgSemaphores[computeIndex];
    signalInfo2.stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    std::vector<VkSemaphoreSubmitInfo> waitInfos{}; //cg, ia
    std::vector<VkSemaphoreSubmitInfo> signalInfos = {signalInfo1, signalInfo2}; //rf, gc
    if (!firstCompute) {
        waitInfos = { waitInfo1, waitInfo2 };
    }
    firstCompute = false;
    if (firstComputeCycle) {
        waitInfos = { waitInfo1 };
    }

    //for compute
    //wait: cc[-1], gc //cc ensures no compute overlap //gc ensures no compute runaway /desync with graphics.
    //signal cc[0], cg  

    VkSubmitInfo2 submitInfo2{};
    submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo2.commandBufferInfoCount = 1;
    submitInfo2.pCommandBufferInfos = &commandBufferInfo;
    submitInfo2.waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size());
    submitInfo2.pWaitSemaphoreInfos = waitInfos.data();
    submitInfo2.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
    submitInfo2.pSignalSemaphoreInfos = signalInfos.data();
    //vkQueueSubmit2(graphicsQueue, 1, &submitInfo, nullptr);

    //spin until compute submit set to false by graphics

    if (vkQueueSubmit2(computeQueue, 1, &submitInfo2, cfFences[computeIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to submit draw command buffer"); }


    computeIndex = (computeIndex + 1) % FRAMES_IN_FLIGHT;
    if (computeIndex == FRAMES_IN_FLIGHT - 1) {
        firstComputeCycle = false;
    }
}


//creation functions
void VulkanEngine::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GravSim";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GravSimEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    /*

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};



    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Instance");
    } */

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;  
    extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();;
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = &debugCreateInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vulkan instance");
    };

}
void VulkanEngine::createLogicalDevice() {
    QueueFamilyIndices indices = findGraphicsQueueFamilies(physicalDevice);
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties;
    queueProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProperties.data());
    queueFamilyCount = 0;
    for (VkQueueFamilyProperties queueProperty : queueProperties) {
        uint32_t flags = queueProperty.queueFlags;
        if ((flags & VK_QUEUE_COMPUTE_BIT)) {
            queueFamilyCount++;
        }
    }
    lowPerformanceSetting = (queueFamilyCount > 1) ? false : true;


    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    bool seperateCompute = true;
    if (lowPerformanceSetting) {
        seperateCompute = false;
    }
    float queuePriority = 0.5f;
    float computePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    if (lowPerformanceSetting) {
  
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = 0;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    else if (seperateCompute) {
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = 0;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = findComputeQueueFamily(physicalDevice);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &computePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    else {
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        settings.configureDeviceFeatures(&requiredDeviceFeatures);

        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = findComputeQueueFamily(physicalDevice);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = findTransferQueueFamily(physicalDevice);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }



    VkPhysicalDeviceSynchronization2Features extraFeatures{};
    extraFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    extraFeatures.synchronization2 = VK_TRUE;

    settings.configureDeviceFeatures(&requiredDeviceFeatures);

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkPhysicalDeviceFeatures standard = requiredDeviceFeatures;

    VkPhysicalDeviceVulkan13Features extras2{};
    extras2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    extras2.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures2 extras{};
    extras.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    extras.features = standard;
    extras.pNext = &extras2;

    createInfo.pNext = &extras;


    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    if (lowPerformanceSetting) {
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        presentQueue = graphicsQueue;
        computeQueue = graphicsQueue;
        transferQueue = graphicsQueue;
    }
    if (seperateCompute) {
        vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
        vkGetDeviceQueue(device, findComputeQueueFamily(physicalDevice), 0, &computeQueue);
        presentQueue = graphicsQueue;
        transferQueue = graphicsQueue;
    }
    else {
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        /*
        vkGetDeviceQueue(device, findComputeQueueFamily(physicalDevice), 0, &computeQueue);
        vkGetDeviceQueue(device, findTransferQueueFamily(physicalDevice), 0, &transferQueue);
        */
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &computeQueue);
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &transferQueue);
    }

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
}
void VulkanEngine::createSurface() {
    if (glfwCreateWindowSurface(instance, winmanager.window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
}
void VulkanEngine::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 2;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findGraphicsQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    swapChainAspectRatio = float(extent.height) / float(extent.width);
}
void VulkanEngine::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}
void VulkanEngine::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;



    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}
void VulkanEngine::createCommandPools() {
    QueueFamilyIndices indices = findGraphicsQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
    
    VkCommandPoolCreateInfo computeInfo{};
    computeInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computeInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    computeInfo.queueFamilyIndex = findComputeQueueFamily(physicalDevice);
    //VkCommandPoolCreateInfo transferInfo{};
    //transferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //transferInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //transferInfo.queueFamilyIndex = findTransferQueueFamily(physicalDevice);

    if (vkCreateCommandPool(device, &computeInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
    //if (vkCreateCommandPool(device, &transferInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
    //    throw std::runtime_error("Failed to create command pool");
    //}
    
    transferCommandPool = graphicsCommandPool;
}
void VulkanEngine::createColourResources() {
    VkFormat colourFormat = swapChainImageFormat;
    createImage(swapChainExtent.width, swapChainExtent.height, msaaSamples, colourFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colourImage, colourImageMemory);
    colourImageView = createImageView(colourImage, colourFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    //transitionImageLayout(colourImage, colourFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}
void VulkanEngine::createDepthResources() {
    VkFormat depthFormat = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    createImage(swapChainExtent.width, swapChainExtent.height, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}
void VulkanEngine::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colourImageView,
            depthImageView,
            swapChainImageViews[i]
            
            
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
void VulkanEngine::createDescriptorPool() {

    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT*5);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT*3);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT + 7*COMPUTE_STEPS);


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT * (1 + 2 + 1 + 1 + 1));

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}
void VulkanEngine::createSyncObjects() {
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenInfo{};
    fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (
            vkCreateSemaphore(device, &semInfo, nullptr, &ccSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &cgSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &igSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &gpSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &gcSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenInfo, nullptr, &cfFences[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenInfo, nullptr, &gfFences[i]) != VK_SUCCESS
            ) {
            throw std::runtime_error("Failed to create engine Sync Objects");
        }
    }
}

void VulkanEngine::createCommandBuffers() {
    VkCommandBufferAllocateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    createInfo.commandPool = graphicsCommandPool;
    createInfo.commandBufferCount = FRAMES_IN_FLIGHT;
    createInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    if (vkAllocateCommandBuffers(device, &createInfo, gCommandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate command buffers"); }
    
    createInfo.commandPool = computeCommandPool;
    if (vkAllocateCommandBuffers(device, &createInfo, cCommandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate command buffers"); }


}

void VulkanEngine::allocateMemory() {
    std::vector<MemoryDetails> memRequirements;
    std::vector<uint16_t> counts;
    particleRasterizer.getMemoryRequirements(&memRequirements, &counts);
    uiRasterizer.getMemoryRequirements(&memRequirements, &counts);
    satEngine.getMemoryRequirements(&memRequirements, &counts);


    //now filter and check for duplicate memory types and alignments
    std::vector<MemoryDetails> orderedMemRequirements; //ordered vector of memory requirements
    std::vector<uint16_t> orderedMemCounts; //acts as an indexed list of the chunks of data
    std::vector<bool> orderedFlags = { false }; //vector of flags to see if current item has already been ordered, blocks duplicates
    std::vector<uint16_t> orderedMappings; //a map of ordered position -> unordered position used later;
    orderedFlags.resize(memRequirements.size());
    orderedMappings.reserve(memRequirements.size());
    orderedMemRequirements.reserve(memRequirements.size()); //for improved push_back performance
    for (size_t i = 0; i < memRequirements.size(); i++) {
        if (orderedFlags[i] == false) {
            //if hasn't been flagged already, do a sweep
            uint16_t flagCount = 1;
            orderedMappings.push_back(static_cast<uint32_t>(i));
            orderedMemRequirements.push_back(memRequirements[i]);
            for (size_t j = 0; j < memRequirements.size(); j++) {
                if (memRequirements[i].requirements.memoryTypeBits == memRequirements[j].requirements.memoryTypeBits &&
                    memRequirements[i].requirements.alignment == memRequirements[j].requirements.alignment &&
                    memRequirements[i].flags == memRequirements[j].flags && 
                    memRequirements[j].flags != (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    i != j &&
                    orderedFlags[j] == false /*this one shouldn't be needed*/) {
                    orderedMappings.push_back(static_cast<uint32_t>(j));
                    orderedMemRequirements.push_back(memRequirements[j]);
                    orderedFlags[j] = true;
                    flagCount++;
                }
            }
            orderedMemCounts.push_back(flagCount);
            orderedFlags[i] = true;
        }
    }
    //now that they are ordered merge them into single memory requirements
    size_t k = 0;
    std::vector<MemoryDetails> mergedMemRequirements;
    mergedMemRequirements.resize(orderedMemCounts.size());
    for (size_t i = 0; i < orderedMemCounts.size(); i++) {
        for (size_t j = k; j < orderedMemCounts[i] + k; j++) {
            mergedMemRequirements[i].requirements.alignment = orderedMemRequirements[j].requirements.alignment;
            mergedMemRequirements[i].requirements.memoryTypeBits = orderedMemRequirements[j].requirements.memoryTypeBits;
            mergedMemRequirements[i].requirements.size += orderedMemRequirements[j].requirements.size;
            mergedMemRequirements[i].flags = orderedMemRequirements[j].flags;
        }
        k += orderedMemCounts[i];
    }

    memory.resize(mergedMemRequirements.size());
    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    uint32_t hostCounter = 0;
    uint32_t deviceCounter = 0;

    for (size_t i = 0; i < mergedMemRequirements.size(); i++) {
        //std::cout << "Size: " << mergedMemRequirements[i].requirements.size << " Flags: " << mergedMemRequirements[i].flags << std::endl;
        memoryInfo.allocationSize = mergedMemRequirements[i].requirements.size;
        memoryInfo.memoryTypeIndex = findMemoryType(mergedMemRequirements[i]);
        if (vkAllocateMemory(device, &memoryInfo, nullptr, &memory[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to allocated memory"); }
        //if (mergedMemRequirements[i].flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        //    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Allocated " + std::to_string(uint32_t(mergedMemRequirements[i].requirements.size)) + "\t bytes of host visible memory with alignment " + std::to_string(uint32_t(mergedMemRequirements[i].requirements.alignment)));
        //}
        //else {
        //    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Allocated " + std::to_string(uint32_t(mergedMemRequirements[i].requirements.size)) + "\t bytes of device memory with alignment " + std::to_string(uint32_t(mergedMemRequirements[i].requirements.alignment)));
        //}
        if (mergedMemRequirements[i].flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { hostCounter += static_cast<uint32_t>(mergedMemRequirements[i].requirements.size); }
        else { deviceCounter += static_cast<uint32_t>(mergedMemRequirements[i].requirements.size); }

    }

    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Allocated " + std::to_string(deviceCounter) + " bytes of device local memory");
    stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Allocated " + std::to_string(hostCounter) + " bytes of host visible memory");

    //now create vector of MemInit structs in order of memRequirements.
    memoryContainers.resize(memRequirements.size());
    k = 0;
    uint32_t offsetCounter;
    for (size_t i = 0; i < orderedMemCounts.size(); i++) {
        offsetCounter = 0;
        for (size_t j = k; j < orderedMemCounts[i] + k; j++) {
            uint16_t mappedIndex = orderedMappings[j];
            memoryContainers[mappedIndex].memory = memory[i];
            memoryContainers[mappedIndex].range = static_cast<uint32_t>(orderedMemRequirements[j].requirements.size);
            memoryContainers[mappedIndex].offset = offsetCounter;
            offsetCounter += static_cast<uint32_t>(orderedMemRequirements[j].requirements.size);
        }
        k += orderedMemCounts[i];
    }
    //now finally dispatch all the MemInit structs to subclasses

    std::vector<uint16_t> memOffsets;
    memOffsets.resize(counts.size());
    //now do prefix sum
    std::exclusive_scan(counts.begin(), counts.end(), memOffsets.data(), 0);

    particleRasterizer.initMemory(&memoryContainers[0] + memOffsets[0]);
    uiRasterizer.initMemory(&memoryContainers[0] + memOffsets[1]);
    satEngine.initMemory(&memoryContainers[0] + memOffsets[2]);

}
void VulkanEngine::initSubclassData() {

    VkDeviceMemory stagingMemory;

    std::array<MemoryDetails,3> memRequirements;
    particleRasterizer.initBufferData_A(&memRequirements[0]);
    uiRasterizer.initBufferData_A(&memRequirements[1]);
    satEngine.initBufferData_A(&memRequirements[2]);

    
    //memRequirements[0].flags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements[0].requirements.size + memRequirements[1].requirements.size + memRequirements[2].requirements.size;
    memoryInfo.memoryTypeIndex = findMemoryType(memRequirements[0]);
    if (vkAllocateMemory(device, &memoryInfo, nullptr, &stagingMemory) != VK_SUCCESS) { throw std::runtime_error("Failed to allocated memory"); }

    std::array<MemInit, 3> memInitStructs;

    memInitStructs[0].memory = stagingMemory;
    memInitStructs[0].offset = 0;
    memInitStructs[0].range = static_cast<uint32_t>(memRequirements[0].requirements.size);

    memInitStructs[1].memory = stagingMemory;
    memInitStructs[1].offset = memInitStructs[0].offset + memInitStructs[0].range;
    memInitStructs[1].range = static_cast<uint32_t>(memRequirements[1].requirements.size);

    memInitStructs[2].memory = stagingMemory;
    memInitStructs[2].offset = memInitStructs[1].offset + memInitStructs[1].range;
    memInitStructs[2].range = static_cast<uint32_t>(memRequirements[2].requirements.size);


    VkCommandBufferAllocateInfo commandInfo{};
    commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandInfo.commandBufferCount = 1;
    commandInfo.commandPool = graphicsCommandPool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    

    VkCommandBuffer transferCommandBuffer;

    vkAllocateCommandBuffers(device, &commandInfo, &transferCommandBuffer);

    particleRasterizer.initBufferData_B(transferCommandBuffer, graphicsQueue, memInitStructs[0]);
    uiRasterizer.initBufferData_B(transferCommandBuffer, graphicsQueue, memInitStructs[1]);
    satEngine.initBufferData_B(transferCommandBuffer, graphicsQueue, memInitStructs[2]);

    vkFreeMemory(device, stagingMemory, nullptr);
    vkFreeCommandBuffers(device, graphicsCommandPool, 1, &transferCommandBuffer);
}



//utility functions
void VulkanEngine::pickPhysicalDevice() {
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }
    devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    deviceScoreMax = 0;
    deviceScores.resize(devices.size());
    for (int i = 0; i < devices.size(); i++) {
        deviceScores[i] = isDeviceSuitable(devices[i]);
        if (deviceScoreMax < deviceScores[i]) {
            deviceScoreMax = deviceScores[i];
        }
    }
    for (int i = 0; i < devices.size(); i++) {
        if (deviceScoreMax == deviceScores[i]) {
            physicalDevice = devices[i];
            VkPhysicalDeviceProperties prop;
            vkGetPhysicalDeviceProperties(devices[i], &prop);
            //std::cout << "Device Picked is " << prop.deviceName << std::endl;
            //std::cout << "Max vulkan version is " << prop.apiVersion << std::endl;
            std::string nameStr;
            uint32_t breakChar = 0;
            for (uint32_t i = 255; i > 0; i--) {
                if (prop.deviceName[i] != 0) {
                    breakChar = i;
                    break;
                }
            }
            for (uint32_t i = 0; i <= breakChar; i++) {
                nameStr.push_back(prop.deviceName[i]);
            }
            stat->addMessage(MSG_LEVEL_STARTUP, "Device picked is " + nameStr);
            stat->addMessage(MSG_LEVEL_DEBUG, "Max Vulkan version is " + std::to_string(prop.apiVersion));
            
        }
    }
}
QueueFamilyIndices VulkanEngine::findGraphicsQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    //int d = 0;
    //for (const auto& queueFamily : queueFamilies) {
    //    d++;
    //    std::cout <<d<< " Queue number: " << queueFamily.queueCount << std::endl;
    //    uint32_t flags = (uint32_t)queueFamily.queueFlags;
    //    std::cout << "Graphics: " << (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) << " Compute: " << (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) << " Transfer: " << (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) << " Sparse Binding: " << (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) << " Queue Protected: "<< (queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT) << std::endl;
    //    //std::cout << "Queue Flags: " << queueFamily.queueFlags << std::endl;
    //}

    std::bitset<32> g(VK_QUEUE_GRAPHICS_BIT);
    std::bitset<32> c(VK_QUEUE_COMPUTE_BIT);
    std::bitset<32> t(VK_QUEUE_TRANSFER_BIT);
    //std::cout << "GRAPHICS: " << g <<  " COMPUTE: " << c << " TRANSFER : " << t << std::endl;



    for (const auto& queueFamily : queueFamilies) {
        std::bitset<32> flags(queueFamily.queueFlags);
        //std::cout << "Index: "<<i<<" Flags: "<<flags <<" Count: " << queueFamily.queueCount <<std::endl;
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}
uint32_t VulkanEngine::findComputeQueueFamily(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    std::vector<uint32_t>priorities;
    uint32_t i = 0;
    for (auto queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                priorities.push_back(1);
            }
            else {
                priorities.push_back(2);
            }
        }
        else { priorities.push_back(0); };
    }
    uint32_t max = 0;
    uint32_t index = 0;
    for (i = 0; i < priorities.size(); i++) {
        if (max < priorities[i]) {
            max = priorities[i];
            index = i;
        }
    }
    if (max != 0) {
        //std::cout << "Compute: " << index << std::endl;
        return index;
    }
    else {
        throw std::runtime_error("Failed to find Compute Queue Families");
    }



}
uint32_t VulkanEngine::findTransferQueueFamily(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    std::vector<uint32_t>priorities;
    uint32_t i = 0;
    for (auto queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                priorities.push_back(1);
            }
            else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                priorities.push_back(1);
            }
            else {
                priorities.push_back(2);
            }
        }
        else { priorities.push_back(0); };
    }
    uint32_t max = 0;
    uint32_t index = 0;
    for (i = 0; i < priorities.size(); i++) {
        if (max < priorities[i]) {
            max = priorities[i];
            index = i;
        }
    }
    if (max != 0) {
        //std::cout << "Transfer: " << index << std::endl;
        return index;
    }
    else {
        throw std::runtime_error("Failed to find Compute Queue Families");
    }



}
SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }


    return details;
}
int VulkanEngine::isDeviceSuitable(VkPhysicalDevice device) {
    int score = 0;
    VkPhysicalDeviceSubgroupProperties subgroupProperties{};
    subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    VkPhysicalDeviceProperties deviceProperties{};
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &subgroupProperties;
    deviceProperties2.properties = deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties2(device, &deviceProperties2);
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    //std::cout << "Subgroup Size: " << subgroupProperties.subgroupSize << std::endl;


    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }
    score += deviceProperties.limits.maxImageDimension2D;
    if (!deviceFeatures.geometryShader) {
        return 0;
    }
    QueueFamilyIndices indices = findGraphicsQueueFamilies(device);
    bool swapChainAdequate = false;

    if (!indices.isComplete()) {
        return 0;
    }
    if (!checkDeviceExtensionSupport(device)) {
        return 0;
    }
    if (settings.Anisotropy) {
        if (!deviceFeatures.samplerAnisotropy) {
            return 0;
        }
    }
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        return 0;
    }
    return score;
}
bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}
VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(winmanager.window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
void VulkanEngine::createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples,  VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    MemoryDetails memDetails{};
    memDetails.requirements = memRequirements;
    memDetails.flags = 0;
    allocInfo.memoryTypeIndex = findMemoryType(memDetails);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
VkImageView VulkanEngine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}
std::vector<const char*> VulkanEngine::getRequiredExtensions() {
    extensions = { winmanager.glfwExtensions, winmanager.glfwExtensions + winmanager.glfwExtensionCount };

    if (enableValidationLayers) {
        ;
    }
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
   
    return extensions;
}
uint32_t VulkanEngine::findMemoryType(MemoryDetails details) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t memoryCount = memProperties.memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) {
        uint32_t memoryTypeBits = (1 << memoryIndex);
        bool isRequiredMemoryType = details.requirements.memoryTypeBits & memoryTypeBits;

        VkMemoryPropertyFlags properties = memProperties.memoryTypes[memoryIndex].propertyFlags;
        bool hasRequiredProperties = (properties & details.flags) == details.flags;

        if (isRequiredMemoryType && hasRequiredProperties) {
            return static_cast<int32_t>(memoryIndex);
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");

}
VkFormat VulkanEngine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported depth format");
}

//command utilities
VkCommandBuffer VulkanEngine::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = transferCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void VulkanEngine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue);

    vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
}
void VulkanEngine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::runtime_error("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}


//valaidation layers and debugging support
void VulkanEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = stat;
}
void VulkanEngine::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}
bool VulkanEngine::checkValidationLayerSupport() {
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    availableLayers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}
VkResult VulkanEngine::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void VulkanEngine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


//runtime functions
void VulkanEngine::recreateSwapChain() {

    int width = 0, height = 0;
    glfwGetFramebufferSize(winmanager.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(winmanager.window, &width, &height);
        if (glfwWindowShouldClose(winmanager.window)) {
            return;
        }
        glfwWaitEvents();

    }
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createColourResources();
    createDepthResources();
    createFramebuffers();
}




//cleanup
void VulkanEngine::cleanupSwapChain() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroyImageView(device, colourImageView, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, colourImage, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, colourImageMemory, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}
void VulkanEngine::cleanup() {
    vkDeviceWaitIdle(device);

    particleRasterizer.cleanup();
    satEngine.cleanup();
    uiRasterizer.cleanup();


    cleanupSwapChain();




    vkDestroyDescriptorPool(device, descriptorPool, nullptr);


    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, ccSemaphores[i], nullptr);
        vkDestroySemaphore(device, cgSemaphores[i], nullptr);
        vkDestroySemaphore(device, igSemaphores[i], nullptr);
        vkDestroySemaphore(device, gpSemaphores[i], nullptr);
        vkDestroySemaphore(device, gcSemaphores[i], nullptr);
        vkDestroyFence(device, cfFences[i], nullptr);
        vkDestroyFence(device, gfFences[i], nullptr);
    }

    for (size_t i = 0; i < memory.size(); i++) {
        vkFreeMemory(device, memory[i], nullptr);
    }




    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
    vkDestroyCommandPool(device, computeCommandPool, nullptr);
    //vkDestroyCommandPool(device, computeCommandPool, nullptr);
    //vkDestroyCommandPool(device, transferCommandPool, nullptr);


    vkDestroyRenderPass(device, renderPass, nullptr);



    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    winmanager.cleanup();

}
