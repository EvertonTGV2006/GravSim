#include "engine.h"

#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
#define GLM_FORCE_RADIANS
#define GLFM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <thread>
#include <chrono>

//for chooseSwapExtent();
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp



void VulkanEngine::initEngine() {
    winmanager.initWindow();
    player->winmanager = winmanager;
    player->updateGLFWcallbacks();
    
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
    createDepthResources();
    createFramebuffers();
    createSyncObjects();
    createCommandBuffers();

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    std::vector<Mesh> meshes;
    meshes.resize(1);
    meshes[0].vertices = &vertices;
    meshes[0].indices = &indices;
    SphereGeometry sphere{};
    sphere.vertices = meshes[0].vertices;
    sphere.indices = meshes[0].indices;
    
    std::thread spheret(&SphereGeometry::createSphereIcosphere, &sphere, 0);

    ParticleGeometry part{};
    part.particles = &particles;
    part.offsets = &offsets;

    std::thread partt(&ParticleGeometry::createParticles, &part, partCount);

    std::vector<std::vector<char>> shaderCode;
    std::vector<std::string> shaderFiles;

    shaderFiles.insert(std::end(shaderFiles), std::begin(gravEngine.shaderFiles), std::end(gravEngine.shaderFiles));
    shaderFiles.insert(std::end(shaderFiles), std::begin(baseRasterizer.shaderFiles), std::end(baseRasterizer.shaderFiles));

    partt.join();

    readFiles(shaderFiles, &shaderCode);

    GravInit grav{};
    grav.commandPool = computeCommandPool;
    grav.descriptorPool = descriptorPool;
    grav.device = device;
    grav.gravQueue = computeQueue;
    grav.memProperties = memProperties;
    grav.particles = &particles;
    grav.offsets = &offsets;
    grav.shaderCode = { &shaderCode[0], &shaderCode[1], &shaderCode[2], &shaderCode[3], &shaderCode[4], &shaderCode[5]};


    //std::thread gravt(&GravEngine::initGrav, &gravEngine, grav);
    gravEngine.initGrav_A(grav);

    spheret.join();

    meshes[0].vertexCount = meshes[0].vertices->size();
    meshes[0].indexCount = meshes[0].indices->size();
    //gravt.join();

    RastInit rast{};
    rast.device = device;
    rast.descriptorPool = descriptorPool;
    rast.renderPass = renderPass;
    rast.memProperties = memProperties;
    rast.meshes = meshes;
    rast.particleCount = partCount;
    rast.shaderCode = { &shaderCode[6], &shaderCode[7] };
    rast.gravStorageBuffer = gravEngine.getInterleavedStorageBuffer();

    //std::thread rastt(&BaseRasterizer::initRast, &baseRasterizer, rast);
    baseRasterizer.initRast_A(rast);
    
    //rastt.join();


    baseRasterizer.storeGravStorageBuffer(gravEngine.getInterleavedStorageBuffer());

    allocateMemory();

    baseRasterizer.initRast_B();
    gravEngine.initGrav_B();

    renderGravSemaphores = gravEngine.getInterleavedSemaphores(gravRenderSemaphores);

    initSubclassData();

    //gravEngine.createRandomData();

    frameTimes.reserve(1000);
}

void VulkanEngine::readFiles(std::vector<std::string> files, std::vector<std::vector<char>>* code) {
    code->resize(files.size());
    for (size_t i = 0; i < files.size(); i++) {
        std::ifstream file(files[i], std::ios::ate | std::ios::binary);

        std::cout << files[i] << std::endl;
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file");
        }
        size_t fileSize = (size_t)file.tellg();
        (*code)[i].resize(fileSize);
        file.seekg(0);
        file.read((*code)[i].data(), fileSize);
        file.close();
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
        executeGraphics();
    }
}
void VulkanEngine::executeGraphics() {



    vkWaitForFences(device, 1, &flightFences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &flightFences[frameIndex]);
    vkResetCommandBuffer(drawCommandBuffers[frameIndex], 0);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
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
    if (vkBeginCommandBuffer(drawCommandBuffers[frameIndex], &beginInfo) != VK_SUCCESS) { throw std::runtime_error("Failed to start draw recording"); }

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
    vkCmdBeginRenderPass(drawCommandBuffers[frameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(drawCommandBuffers[frameIndex], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(drawCommandBuffers[frameIndex], 0, 1, &scissor);

    //CameraPushConstants camera{};
    //camera.cameraPos = glm::vec4(player->pos, 1.0);
    //camera.viewDirection = glm::vec4(player->viewDirection, 1.0f);
    //camera.lightColour = { 1.0f, 1.0f, 1.0f, 0.0f };
    //camera.lightPos = { 2.0f, 0.0f, 3.0f , 0.0f };

    player->updatePlayerMovement();
    player->updateViewMat();
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1);
    ubo.view = player->viewMat;
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 1000.0f);
    ubo.zeta = glm::mat4(1);

    baseRasterizer.drawObjects(drawCommandBuffers[frameIndex], frameIndex, ubo);

    vkCmdEndRenderPass(drawCommandBuffers[frameIndex]);

    if (vkEndCommandBuffer(drawCommandBuffers[frameIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to record draw"); }

    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = drawCommandBuffers[frameIndex];

    

    VkSemaphoreSubmitInfo waitInfo1{};
    waitInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo1.semaphore = imageSemaphores[frameIndex];
    waitInfo1.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo waitInfo2{};
    waitInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo2.semaphore = gravRenderSemaphores[frameIndex];
    waitInfo2.stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    VkSemaphoreSubmitInfo signalInfo1{};
    signalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo1.semaphore = renderSemaphores[frameIndex];
    signalInfo1.stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSemaphoreSubmitInfo signalInfo2{};
    signalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo2.semaphore = renderGravSemaphores[(frameIndex+1)%FRAMES_IN_FLIGHT];
    signalInfo2.stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    std::array<VkSemaphoreSubmitInfo, 2> waitInfos = { waitInfo1 , waitInfo2};
    std::array<VkSemaphoreSubmitInfo, 2> signalInfos = { signalInfo1 , signalInfo2};

    VkSubmitInfo2 submitInfo2{};
    submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo2.commandBufferInfoCount = 1;
    submitInfo2.pCommandBufferInfos = &commandBufferInfo;
    submitInfo2.waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size());
    submitInfo2.pWaitSemaphoreInfos = waitInfos.data();
    submitInfo2.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
    submitInfo2.pSignalSemaphoreInfos = signalInfos.data();
    //vkQueueSubmit2(graphicsQueue, 1, &submitInfo, nullptr);

    if (firstFrame) {
        submitInfo2.waitSemaphoreInfoCount = 1;
        submitInfo2.pWaitSemaphoreInfos = &waitInfo1;
        firstFrame = false;
    }

    if (vkQueueSubmit2(graphicsQueue, 1, &submitInfo2, flightFences[frameIndex]) != VK_SUCCESS) { throw std::runtime_error("Failed to submit draw command buffer"); }
    

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSemaphore signalSemaphores[] = { renderSemaphores[frameIndex] };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
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

    ct = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration<double>(ct - pt);
    pt = ct;
    gravEngine.simGrav(dt.count());

    frameTimes.push_back(dt);

    if (frameTimes.size() % 200 == 0) {
        std::cout << "Player Pos: ";
        glm::vec3 v = player->pos;
        std::cout << v.x << ", " << v.y << ", " << v.z;
        std::cout << std::endl;
    }

    if (gravEngine.endTrigger) {
        glfwSetWindowShouldClose(winmanager.window, GLFW_TRUE);
    }

    //gravEngine.createRandomData();

    //std::cout << "FPS: " << 1.0 / dt.count() << std::endl;

    frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}
void VulkanEngine::runCompute() {
    while (!glfwWindowShouldClose(winmanager.window)) {
        executeCompute();
    }
}
void VulkanEngine::executeCompute() {
    std::cout << "Compute" << std::endl;
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

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    settings.configureDeviceFeatures(&requiredDeviceFeatures);
    VkDeviceQueueCreateInfo queueCreateInfo{};
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
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    /*
    vkGetDeviceQueue(device, findComputeQueueFamily(physicalDevice), 0, &computeQueue);
    vkGetDeviceQueue(device, findTransferQueueFamily(physicalDevice), 0, &transferQueue);
    */
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &computeQueue);
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &transferQueue);

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
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
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
    /*
    VkCommandPoolCreateInfo computeInfo{};
    computeInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computeInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    computeInfo.queueFamilyIndex = findComputeQueueFamily(physicalDevice);
    VkCommandPoolCreateInfo transferInfo{};
    transferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    transferInfo.queueFamilyIndex = findTransferQueueFamily(physicalDevice);

    if (vkCreateCommandPool(device, &computeInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
    if (vkCreateCommandPool(device, &transferInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
    */
    transferCommandPool = graphicsCommandPool;
    computeCommandPool = graphicsCommandPool;
}
void VulkanEngine::createDepthResources() {
    VkFormat depthFormat = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}
void VulkanEngine::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
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
    poolSizes[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT + COMPUTE_STEPS);


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT +2* COMPUTE_STEPS);

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
            vkCreateSemaphore(device, &semInfo, nullptr, &imageSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &renderSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &gravRenderSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenInfo, nullptr, &flightFences[i]) != VK_SUCCESS
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
    if (vkAllocateCommandBuffers(device, &createInfo, drawCommandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("Failed to allocate command buffers"); }
}

void VulkanEngine::allocateMemory() {
    std::vector<MemoryDetails> memRequirements;
    std::vector<uint16_t> counts;
    gravEngine.getMemoryRequirements(&memRequirements, &counts);
    baseRasterizer.getMemoryRequirements(&memRequirements, &counts);


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
            orderedMappings.push_back(i);
            orderedMemRequirements.push_back(memRequirements[i]);
            for (size_t j = 0; j < memRequirements.size(); j++) {
                if (memRequirements[i].requirements.memoryTypeBits == memRequirements[j].requirements.memoryTypeBits &&
                    memRequirements[i].requirements.alignment == memRequirements[j].requirements.alignment &&
                    memRequirements[i].flags == memRequirements[j].flags &&
                    i != j &&
                    orderedFlags[j] == false /*this one shouldn't be needed*/) {
                    orderedMappings.push_back(j);
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
    for (size_t i = 0; i < mergedMemRequirements.size(); i++) {
        std::cout << "Size: " << mergedMemRequirements[i].requirements.size << " Flags: " << mergedMemRequirements[i].flags << std::endl;
        memoryInfo.allocationSize = mergedMemRequirements[i].requirements.size;
        memoryInfo.memoryTypeIndex = findMemoryType(mergedMemRequirements[i]);
        if (vkAllocateMemory(device, &memoryInfo, nullptr, &memory[i]) != VK_SUCCESS) { throw std::runtime_error("Failed to allocated memory"); }

    }
    //now create vector of MemInit structs in order of memRequirements.
    memoryContainers.resize(memRequirements.size());
    k = 0;
    uint32_t offsetCounter;
    for (size_t i = 0; i < orderedMemCounts.size(); i++) {
        offsetCounter = 0;
        for (size_t j = k; j < orderedMemCounts[i] + k; j++) {
            uint16_t mappedIndex = orderedMappings[j];
            memoryContainers[mappedIndex].memory = memory[i];
            memoryContainers[mappedIndex].range = orderedMemRequirements[j].requirements.size;
            memoryContainers[mappedIndex].offset = offsetCounter;
            offsetCounter += orderedMemRequirements[j].requirements.size;
        }
        k += orderedMemCounts[i];
    }
    //now finally dispatch all the MemInit structs to subclasses
    gravEngine.initMemory({ memoryContainers[0], memoryContainers[1],memoryContainers[2],memoryContainers[3] });
    baseRasterizer.initMemory({ memoryContainers[4],memoryContainers[5],memoryContainers[6]});

}
void VulkanEngine::initSubclassData() {

    VkDeviceMemory stagingMemory;

    std::array<MemoryDetails,2> memRequirements;
    baseRasterizer.initBufferData_A(&memRequirements[0]);
    gravEngine.syncBufferData_A(&memRequirements[1]);
    //memRequirements[0].flags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements[0].requirements.size + memRequirements[1].requirements.size;
    memoryInfo.memoryTypeIndex = findMemoryType(memRequirements[0]);
    if (vkAllocateMemory(device, &memoryInfo, nullptr, &stagingMemory) != VK_SUCCESS) { throw std::runtime_error("Failed to allocated memory"); }

    std::array<MemInit, 2> memInitStructs;

    memInitStructs[0].memory = stagingMemory;
    memInitStructs[0].offset = 0;
    memInitStructs[0].range = memRequirements[0].requirements.size;

    memInitStructs[1].memory = stagingMemory;
    memInitStructs[1].offset = memInitStructs[0].offset + memInitStructs[0].range;
    memInitStructs[1].range = memRequirements[1].requirements.size;

    VkCommandBufferAllocateInfo commandInfo{};
    commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandInfo.commandBufferCount = 1;
    commandInfo.commandPool = graphicsCommandPool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    

    VkCommandBuffer transferCommandBuffer;

    vkAllocateCommandBuffers(device, &commandInfo, &transferCommandBuffer);

    baseRasterizer.initBufferData_B(transferCommandBuffer, graphicsQueue, memInitStructs[0]);
    gravEngine.syncBufferData_B(true, memInitStructs[1]);


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
            std::cout << "Device Picked is " << prop.deviceName << std::endl;
            std::cout << "Max vulkan version is" << prop.apiVersion << std::endl;
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
    std::cout << "GRAPHICS: " << g <<  " COMPUTE: " << c << " TRANSFER : " << t << std::endl;



    for (const auto& queueFamily : queueFamilies) {
        std::bitset<32> flags(queueFamily.queueFlags);
        std::cout << "Index: "<<i<<" Flags: "<<flags <<" Count: " << queueFamily.queueCount <<std::endl;
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
        std::cout << "Compute: " << index << std::endl;
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
        std::cout << "Transfer: " << index << std::endl;
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

    std::cout << "Subgroup Size: " << subgroupProperties.subgroupSize << std::endl;


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
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
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
void VulkanEngine::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
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
        throw std::invalid_argument("unsupported layout transition!");
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
    createDepthResources();
    createFramebuffers();
}


void VulkanEngine::writeOutSampleData() {
    std::ofstream dataOut;
    dataOut.open("data.csv", std::ios::app);
    if (!dataOut.is_open()) {
        throw std::runtime_error("Failed to open file for writing");
    }
    dataOut << runNumber << ", ";
    for (size_t i = 1; i < frameTimes.size(); i++) {
        dataOut << frameTimes[i].count() << ", ";
    }
    dataOut << "\n";
    dataOut.close();
}

//cleanup
void VulkanEngine::cleanupSwapChain() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}
void VulkanEngine::cleanup() {
    vkDeviceWaitIdle(device);
    baseRasterizer.cleanup();
    gravEngine.cleanup();

    writeOutSampleData();

    cleanupSwapChain();




    vkDestroyDescriptorPool(device, descriptorPool, nullptr);


    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageSemaphores[i], nullptr);
        vkDestroySemaphore(device, gravRenderSemaphores[i], nullptr);
        vkDestroyFence(device, flightFences[i], nullptr);
    }

    for (size_t i = 0; i < memory.size(); i++) {
        vkFreeMemory(device, memory[i], nullptr);
    }




    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
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
