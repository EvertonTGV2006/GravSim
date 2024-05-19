
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "window.h"
#include "renderer.h"
#include "structs.h"
#include "geometry.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <array>
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


QueueFamilyIndices VulkanRenderer::findGraphicsQueueFamilies(VkPhysicalDevice device) {
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

    for (const auto& queueFamily : queueFamilies) {
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
uint32_t VulkanRenderer::findComputeQueueFamily(VkPhysicalDevice device) {
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
uint32_t VulkanRenderer::findTransferQueueFamily(VkPhysicalDevice device) {
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
        std::cout <<"Transfer: "<< index << std::endl;
        return index;
    }
    else {
        throw std::runtime_error("Failed to find Compute Queue Families");
    }



}
SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device) {
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


void VulkanRenderer::initVulkan() {
    SphereGeometry sphere;
    sphere.mode = 0;
    sphere.vertices = &vertices;
    sphere.indices = &indices;
    //sphere.createIcosahedron(true);
    sphere.createSphereIcosphere(1);
    std::cout <<"Indices: " <<indices.size()<<" Vertices: "<<vertices.size()<<std::endl;
    //for (uint16_t cursor = 0; cursor < indices.size(); cursor += 3) {
    //    std::cout << "Triangle: " << cursor / 3 << " | " << indices[cursor] << " " << indices[cursor + 1] << " " << indices[cursor + 2] << std::endl;
    //}

    ParticleGeometry part;
    particles.clear();
    std::cout << "Particles capacity: " << particles.max_size() << std::endl;
    part.particles = &particles;
    part.createParticles(16384);

    std::cout << std::endl << "Particle: " << sizeof(Particle) << std::endl;
    std::cout << "Position: " << sizeof(particles[0].position) << " " << offsetof(Particle, position) << std::endl;;
    std::cout << "Velocity: " << sizeof(particles[0].velocity) << " " << offsetof(Particle, velocity) << std::endl;;
    std::cout << "Mass: " << sizeof(particles[0].mass) << " " << offsetof(Particle, mass) << std::endl;;

    std::cout << particles.size() << std::endl;;
    //for (auto part : (particles)) {
    //    std::cout << "Position: " << part.position.x << " | " << part.position.y << " | " << part.position.z << " | " << glm::sqrt(glm::dot(part.position, part.position)) << std::endl;
    //    std::cout << "Velocity: " << part.velocity.x << " | " << part.velocity.y << " | " << part.velocity.z << " | " << glm::sqrt(glm::dot(part.velocity, part.velocity)) << std::endl;
    //}

	createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createComputePipeline();
    createCommandPools();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createComputeBuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
    
}




void VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GravSim";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GravSimEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

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
    }
}

void VulkanRenderer::pickPhysicalDevice() {
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
            std::cout << "Max vulkan version is " << prop.apiVersion << std::endl;


            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{};
            rt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            features2.pNext = &rt;
            vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
            std::cout << "Ray Tracing Support: "<<rt.rayTracingPipeline << std::endl;
            std::cout << VK_TRUE << std::endl;
        }
    }
}
int VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device) {
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

    std::cout << "Subgroup Size: "<<subgroupProperties.subgroupSize << std::endl;


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
bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
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
VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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


void VulkanRenderer::createLogicalDevice() {
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

    VkPhysicalDeviceSynchronization2Features extraFeatures;
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
    vkGetDeviceQueue(device, findComputeQueueFamily(physicalDevice), 0, &computeQueue);
    vkGetDeviceQueue(device, findTransferQueueFamily(physicalDevice), 0, &transferQueue);
}
void VulkanRenderer::createSurface() {
    if (glfwCreateWindowSurface(instance, winmanager.window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
}
void VulkanRenderer::createSwapChain() {
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
    //std::cout << swapChainImageFormat << std::endl;
    //VK_FORMAT_R8G8B8A8_SRGB;
    swapChainExtent = extent;
}
void VulkanRenderer::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanRenderer::createGraphicsPipeline() {
    if (vertShaderCode.size() < 10 || fragShaderCode.size() < 10) {
        throw std::runtime_error("Shaders too small");
    }
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &graphicsDescriptorSetLayout;

    VkPushConstantRange cameraConstants{};
    cameraConstants.offset = 0;
    cameraConstants.size = static_cast<uint32_t>(sizeof(CameraPushConstants));
    cameraConstants.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    VkPushConstantRange modelConstants{};
    modelConstants.offset = static_cast<uint32_t>(sizeof(CameraPushConstants));
    modelConstants.size = static_cast<uint32_t>(sizeof(ModelPushConstants));
    modelConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkPushConstantRange, 2> pushConstantRanges = { cameraConstants, modelConstants};

    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();



    //VkPushConstantRange modelConstants{};
    //modelConstants.offset = 48;
    //cameraConstants.size = sizeof(ModelPushConstants);
    //cameraConstants.stageFlags= VK_SHADER_STAGE_VERTEX_BIT;

    //VkPushConstantRange lightConstants{};
    //lightConstants.offset = sizeof(CameraPushConstants) + sizeof(ModelPushConstants);
    //lightConstants.size = sizeof(LightingPushConstants);
    //lightConstants.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    //std::array<VkPushConstantRange, 1> pushConsatants = { cameraConstants };
    //pipelineLayoutInfo.pPushConstantRanges = pushConsatants.data();
    //pipelineLayoutInfo.pushConstantRangeCount = pushConsatants.size();

    //VkPushConstantRange testRange{};
    //testRange.offset = 0;
    //testRange.size = sizeof(float);
    //testRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    



    //pipelineLayoutInfo.pushConstantRangeCount = 1;
    //pipelineLayoutInfo.pPushConstantRanges = &testRange;

    //pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = graphicsPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
void VulkanRenderer::createComputePipeline() {
    VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    VkPushConstantRange computeConst{};
    computeConst.size = sizeof(ComputeConstants);
    computeConst.offset = 0;
    computeConst.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo computeLayoutInfo{};
    computeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayoutInfo.setLayoutCount = 1;
    computeLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;
    computeLayoutInfo.pushConstantRangeCount = 1;
    computeLayoutInfo.pPushConstantRanges = &computeConst;

    if (vkCreatePipelineLayout(device, &computeLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline layout");
    }

    

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = computePipelineLayout;
    computePipelineInfo.stage = computeShaderStageInfo;
    computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
       
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Compute Pipeline");
    }

    vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void VulkanRenderer::createRenderPass() {
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
    std::cout <<"Image Format: "<< colorAttachment.format << std::endl;
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
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

void VulkanRenderer::createFramebuffers() {
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
void VulkanRenderer::createCommandPools() {
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
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer " + (char)usage);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};

    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory " + (char)usage);
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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

void VulkanRenderer::createDepthResources() {
    VkFormat depthFormat = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}
void VulkanRenderer::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize)); 
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

}
void VulkanRenderer::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}
void VulkanRenderer::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (settings.Anisotropy) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    }
    else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler");
    }
}
void VulkanRenderer::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void VulkanRenderer::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize= sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}
void VulkanRenderer::createComputeBuffers() {
    computeBuffers.resize(MAX_COMPUTE_FRAMES);
    computeBufferMemory.resize(MAX_COMPUTE_FRAMES);

    VkDeviceSize bufferSize = static_cast<uint32_t>(sizeof(particles[0]) * particles.size());
    void* data;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, particles.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);



    for (size_t i = 0; i < MAX_COMPUTE_FRAMES; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, computeBuffers[i], computeBufferMemory[i]);
        copyBuffer(stagingBuffer, computeBuffers[i], bufferSize);
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    transferBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    transferBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, transferBuffers[i], transferBufferMemory[i]);
    }
}
void VulkanRenderer::createDescriptorPool() {

    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + MAX_COMPUTE_FRAMES);


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT + MAX_COMPUTE_FRAMES);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}
void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, graphicsDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    graphicsDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, graphicsDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkDescriptorSetLayout> computeLayouts(MAX_COMPUTE_FRAMES, computeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo computeAllocInfo{};
    computeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeAllocInfo.descriptorPool = descriptorPool;
    computeAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_COMPUTE_FRAMES);
    computeAllocInfo.pSetLayouts = computeLayouts.data();

    computeDescriptorSets.resize(MAX_COMPUTE_FRAMES);
    if (vkAllocateDescriptorSets(device, &computeAllocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate compute descriptor sets");
    }


    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkDescriptorBufferInfo ssboInfo{};
        ssboInfo.buffer = transferBuffers[i];
        ssboInfo.offset = 0;
        ssboInfo.range = static_cast<uint32_t>(sizeof(Particle) * particles.size());

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &ssboInfo;


        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    for (size_t j = 0; j < MAX_COMPUTE_FRAMES; j++) {
        //std::cout << "Compute: " << j << " Read: " << (j + (MAX_COMPUTE_FRAMES - 1)) % MAX_COMPUTE_FRAMES << " Write: " << j << std::endl;
        VkDescriptorBufferInfo lastStorageBuffer{};
        lastStorageBuffer.buffer = computeBuffers[(j+(MAX_COMPUTE_FRAMES-1))%MAX_COMPUTE_FRAMES];
        lastStorageBuffer.offset = 0;
        lastStorageBuffer.range = static_cast<uint32_t>(sizeof(Particle) * particles.size());


        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = computeDescriptorSets[j];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &lastStorageBuffer;

        VkDescriptorBufferInfo currentStorageBuffer{};
        currentStorageBuffer.buffer = computeBuffers[j];
        currentStorageBuffer.offset = 0;
        currentStorageBuffer.range = static_cast<uint32_t>(sizeof(Particle) * particles.size());

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = computeDescriptorSets[j];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &currentStorageBuffer;

        vkUpdateDescriptorSets(device, 2, descriptorWrites.data(), 0, nullptr);
        
    }
}
void VulkanRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding ssboLayoutBinding{};
    ssboLayoutBinding.binding = 2;
    ssboLayoutBinding.descriptorCount = 1;
    ssboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboLayoutBinding.pImmutableSamplers = nullptr;
    ssboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, ssboLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Graphics descriptor set layout");
    }

    std::array < VkDescriptorSetLayoutBinding, 2> layoutBindings{}; 
    
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo computeLayoutInfo{};
    computeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    computeLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    computeLayoutInfo.pBindings = layoutBindings.data();


    if (vkCreateDescriptorSetLayout(device, &computeLayoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute descriptor set layout");
    }
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}
void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
   

    endSingleTimeCommands(commandBuffer);
}
VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, graphicsCommandPool, 1, &commandBuffer);
}
void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
void VulkanRenderer::recordTransferCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index1, uint32_t index2) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to beign transfer command buffer");
    }
    VkBufferCopy bufferInfo{};
    bufferInfo.size = static_cast<uint32_t>(sizeof(Particle) * particles.size());
    vkCmdCopyBuffer(commandBuffer, computeBuffers[index1], transferBuffers[index2], 1, &bufferInfo);
    

    vkEndCommandBuffer(commandBuffer);

}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");

}
VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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
void VulkanRenderer::createCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Command Buffers");
    }
    
    VkCommandBufferAllocateInfo computeInfo{};
    computeCommandBuffers.resize(MAX_COMPUTE_FRAMES);
    computeInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    computeInfo.commandPool = computeCommandPool;
    computeInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    computeInfo.commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size());

    if (vkAllocateCommandBuffers(device, &computeInfo, computeCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate compute command Buffers");
    }


    transferCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo transferInfo{};
    transferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    transferInfo.commandPool = transferCommandPool;
    transferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    transferInfo.commandBufferCount = static_cast<uint32_t>(transferCommandBuffers.size());


    if (vkAllocateCommandBuffers(device, &transferInfo, transferCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate transfer command buffers");
    }


}


void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
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
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &graphicsDescriptorSets[currentFrame], 0, nullptr);


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    CameraPushConstants camera{};
    camera.cameraPos = glm::vec4(player->pos, 1.0);
    camera.viewDirection = glm::vec4(player->viewDirection, 1.0f);
    camera.lightColour = { 1.0f, 1.0f, 1.0f, 0.0f };
    camera.lightPos = { 2.0f, 0.0f, 3.0f , 0.0f};
    ModelPushConstants model{};
    model.modelPos = glm::mat4(1);
    model.mode = 0;

    bool wireframe = false;

    if (wireframe) {
        model.mode = 0;
        model.modelPos *= 20;
        //std::cout << model.modelPos[0][0] << std::endl;
        vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_cast<uint32_t>(sizeof(CameraPushConstants)), &camera);
        vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, static_cast<uint32_t>(sizeof(CameraPushConstants)), static_cast<uint32_t>(sizeof(ModelPushConstants)), &model);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
    else {

        vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_cast<uint32_t>(sizeof(CameraPushConstants)), &camera);
        vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, static_cast<uint32_t>(sizeof(CameraPushConstants)), static_cast<uint32_t>(sizeof(ModelPushConstants)), &model);

        //vkCmdDrawIndexed(commandBuffer, 6, 1, static_cast<uint32_t>(indices.size()) - 6, 0, 0);

        model.mode = 1;
        vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, static_cast<uint32_t>(sizeof(CameraPushConstants)), static_cast<uint32_t>(sizeof(ModelPushConstants)), &model);

        //model.modelPos = glm::rotate(glm::mat4(1.0f), (float)(glfwGetTime() * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));

        //vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, static_cast<uint32_t>(sizeof(CameraPushConstants)), static_cast<uint32_t>(sizeof(ModelPushConstants)), &model);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), static_cast<uint32_t>(particles.size()), 0, 0, 0);

    }

    //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()) - 6, static_cast<uint32_t>(particles.size()), 0, 0, 0);

    //model.modelPos = glm::translate(glm::mat4(1), glm::vec3(3.0f, 2.0f, 1.0f)) * model.modelPos;

    //vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, static_cast<uint32_t>(sizeof(CameraPushConstants)), static_cast<uint32_t>(sizeof(ModelPushConstants)), &model);

    //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()) - 6, 1, 0, 0, 0);

    //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record Command Buffer");
    }
}
void VulkanRenderer::recordComputeCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSets[currentCompute], 0, nullptr);

    double currentTime = computePrevTime;
    computePrevTime = glfwGetTime();

    ComputeConstants constants;
    constants.deltaTime = (computePrevTime-currentTime);
    if (player->timeAccel == true) {
        constants.deltaTime *= 100;
    }
    if (player->timePause == true) {
        constants.deltaTime = 0.5;
    }
    //std::cout << "Delta Time: " << constants.deltaTime << std::endl;
    while (player->timePause == true && player->triggerStep == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        constants.deltaTime = 0.5;
        computePrevTime = glfwGetTime();
    }
    player->triggerStep = false;

    vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeConstants), &constants);
    float workgroupsxraw = (float)particles.size() / (float)WORKGROUP_SIZE;
    uint32_t workgroupsx= std::ceil(workgroupsxraw);
    //std::cout << "WorkGroups!: " << workgroupsx << std::endl;

    //vkCmdDispatch(commandBuffer, workgroupsx, 1, 1);

    vkCmdDispatch(commandBuffer, static_cast<uint32_t>(particles.size()), 1, 1);
    


    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record Command Buffer");
    }
}


void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    UniformBufferObject ubo{};
    player->updatePlayerMovement();
    player->updateViewMat();
    ubo.model = glm::rotate(glm::mat4(1.0f), (float)(glfwGetTime() * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = player->viewMat;
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
void VulkanRenderer::createSyncObjects() {
    imageAvailableSempahores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinihsedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    transferFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    transferFinishedFences.resize(MAX_FRAMES_IN_FLIGHT);
    vertexFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    computeFinishedSemaphores1.resize(MAX_COMPUTE_FRAMES);
    computeFinishedSemaphores2.resize(MAX_COMPUTE_FRAMES);
    computeFences.resize(MAX_COMPUTE_FRAMES);


    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSempahores[i]) != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinihsedSemaphores[i]) != VK_SUCCESS 
            || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &transferFinishedSemaphores[i])!=VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &vertexFinishedSemaphores[i])!=VK_SUCCESS
            || vkCreateFence(device, &fenceInfo, nullptr, &transferFinishedFences[i])!=VK_SUCCESS) {
            throw std::runtime_error("Failed to create Sync Objects");
        }
    }
    for (size_t i = 0; i < MAX_COMPUTE_FRAMES; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinishedSemaphores1[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinishedSemaphores2[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &computeFences[i])!= VK_SUCCESS) {
            throw std::runtime_error("Failed to create sync objects");
        }
    }
}
void VulkanRenderer::recreateSwapChain() {

    int width = 0, height = 0;
    glfwGetFramebufferSize(winmanager.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(winmanager.window, &width, &height);
        if (glfwWindowShouldClose(winmanager.window)) {
            return;
        }
        glfwWaitEvents();

    }
    vkQueueWaitIdle(graphicsQueue);
    vkQueueWaitIdle(presentQueue);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

void VulkanRenderer::queueDraw() {
    //for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i ++ ) {
    //    copyBuffer(computeBuffers[0], transferBuffers[i], static_cast<uint32_t>(sizeof(Particle) * particles.size()));
    //}
    //for (auto part : particles) {
    //    std::cout << "Position: " << part.position.x << " | " << part.position.y << " | " << part.position.z << " | " << glm::sqrt(glm::dot(part.position, part.position)) << std::endl;
    //    std::cout << "Velocity: " << part.velocity.x << " | " << part.velocity.y << " | " << part.velocity.z << " | " << glm::sqrt(glm::dot(part.velocity, part.velocity)) << std::endl;
    //}
    auto start = std::chrono::high_resolution_clock::now();
    auto nextFrameClock = std::chrono::high_resolution_clock::now();
    while (player->windowShouldClose == false) {
        std::this_thread::sleep_until(nextFrameClock);
        nextFrameClock = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
        drawFrame();

    }
}
void VulkanRenderer::queueCompute() {
    while (player->windowShouldClose == false) {
        computeFrame();
    }
}
void VulkanRenderer::queue() {
    std::thread graphics(&VulkanRenderer::queueDraw, this);
    std::thread compute(&VulkanRenderer::queueCompute, this);
    std::cout << "Graphics Thread: " << graphics.get_id() << " | Compute Thread: "<<compute.get_id() << std::endl;
    while (player->windowShouldClose == false) {
        glfwWaitEvents();
    }
    player->timePause = false;

    graphics.join();
    compute.join();
}


void VulkanRenderer::drawFrame() {
    //if (FirstFrame);
    //else {
    //    vkWaitForFences(device, 1, &computeFences[currentCompute], VK_TRUE, UINT64_MAX);
    //}
    //vkResetFences(device, 1, &computeFences[currentCompute]);
    //vkResetCommandBuffer(computeCommandBuffers[currentCompute], 0);
    //
    //recordComputeCommandBuffer(computeCommandBuffers[currentCompute], currentCompute);
    //
    //

    //VkCommandBufferSubmitInfo computeCommandBufferInfo{};
    //computeCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    //computeCommandBufferInfo.commandBuffer = computeCommandBuffers[currentCompute];

    //VkSemaphoreSubmitInfo computeWaitInfo{};
    //computeWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    //computeWaitInfo.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    //computeWaitInfo.semaphore = computeFinishedSemaphores1[currentCompute];

    //VkSemaphoreSubmitInfo computeSignalInfo1{};
    //computeSignalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    //computeSignalInfo1.semaphore = computeFinishedSemaphores1[(currentCompute + 1) % MAX_COMPUTE_FRAMES];
    //computeSignalInfo1.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    //VkSemaphoreSubmitInfo computeSignalInfo2{};
    //computeSignalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    //computeSignalInfo2.semaphore = computeFinishedSemaphores2[(currentCompute + 1) % MAX_COMPUTE_FRAMES];
    //computeSignalInfo2.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    //
    //std::array<VkSemaphoreSubmitInfo, 2> computeSignalInfos = { computeSignalInfo1, computeSignalInfo2 };

    //VkSubmitInfo2 computeSubmitInfo{};
    //computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    //computeSubmitInfo.commandBufferInfoCount = 1;
    //computeSubmitInfo.pCommandBufferInfos = &computeCommandBufferInfo;
    //computeSubmitInfo.waitSemaphoreInfoCount = 1;
    //computeSubmitInfo.pWaitSemaphoreInfos = &computeWaitInfo;
    //computeSubmitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(computeSignalInfos.size());
    //computeSubmitInfo.pSignalSemaphoreInfos = computeSignalInfos.data();


    //if (FirstFrame) {
    //    computeSubmitInfo.waitSemaphoreInfoCount = 0;
    //    computeSubmitInfo.pWaitSemaphoreInfos = nullptr;
    //    std::cout << "First Frame" << std::endl;
    //    FirstFrame = false;
    //}





    //if (vkQueueSubmit2(computeQueue, 1, &computeSubmitInfo, computeFences[(currentCompute) % MAX_COMPUTE_FRAMES])!= VK_SUCCESS) {
    //    throw std::runtime_error("Failed to submit compute work");
    //}

    








    //std::cout << "Graphics starting wait: " << currentFrame << currentCompute;
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    //std::cout << "Graphics wait returned" << std::endl;
    vkWaitForFences(device, 1, &transferFinishedFences[currentFrame], VK_TRUE, UINT64_MAX);

    vkResetFences(device, 1, &transferFinishedFences[currentFrame]);

    vkResetCommandBuffer(transferCommandBuffers[currentFrame], 0);
    //std::cout << "Transfer Request | ";
    //transferRequest = true;
    //while (transferRequest == true) {
    //    std::this_thread::sleep_for(std::chrono::microseconds(1));
    //}
    uint32_t computeIndex = (currentCompute)%MAX_COMPUTE_FRAMES;
    //std::cout << computeIndex << std::endl;
    //std::cout << "Current Compute: "<<computeIndex << std::endl;
    //std::cout << "Transfer Confirmed | ";
    //std::cout << computeIndex;

    recordTransferCommandBuffer(transferCommandBuffers[currentFrame], computeIndex, currentFrame);

    VkCommandBufferSubmitInfo transferBufferInfo{};
    transferBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    transferBufferInfo.commandBuffer = transferCommandBuffers[currentFrame];
    
    //VkSemaphoreSubmitInfo waitInfo{};
    //waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    //waitInfo.semaphore = computeFinishedSemaphores2[computeIndex];
    //waitInfo.stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSemaphoreSubmitInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo.semaphore = transferFinishedSemaphores[currentFrame];
    signalInfo.stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &transferBufferInfo;
    //submitInfo.waitSemaphoreInfoCount = 1;
    //submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;

    submitInfo.waitSemaphoreInfoCount = 0;
    submitInfo.pWaitSemaphoreInfos = nullptr;
    //std::cout << " | Waiting submit | ";

    //while (transferSubmitWait == true) {
    //    std::this_thread::sleep_for(std::chrono::microseconds(1));
    //}
    //transferSubmitWait = true;
    //std::cout << "Submit!"<<std::endl;
    if (vkQueueSubmit2(transferQueue, 1, &submitInfo, transferFinishedFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit transfer buffer");
    }

    

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSempahores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }


    updateUniformBuffer(currentFrame);

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    
    //VkSubmitInfo submitInfo{};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //VkSemaphore waitSemaphores[] = { imageAvailableSempahores[currentFrame]};
    //VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    //submitInfo.waitSemaphoreCount = 1;
    //submitInfo.pWaitSemaphores = waitSemaphores;
    //submitInfo.pWaitDstStageMask = waitStages;
    //submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    //VkSemaphore signalSemaphores[] = { renderFinihsedSemaphores[currentFrame]};
    //submitInfo.signalSemaphoreCount = 1;
    //submitInfo.pSignalSemaphores = signalSemaphores;
    //
    //if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    //    throw std::runtime_error("Failed to submit draw command buffer");
    //}

    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = commandBuffers[currentFrame];

    VkSemaphoreSubmitInfo waitInfo1{};
    waitInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo1.semaphore = imageAvailableSempahores[currentFrame];
    waitInfo1.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo waitInfo2{};
    waitInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo2.semaphore = transferFinishedSemaphores[currentFrame];
    waitInfo2.stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    VkSemaphoreSubmitInfo signalInfo1{};
    signalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo1.semaphore = renderFinihsedSemaphores[currentFrame];
    signalInfo1.stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSemaphoreSubmitInfo signalInfo2{};
    signalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo2.semaphore = vertexFinishedSemaphores[currentFrame];
    signalInfo2.stageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    
    std::array<VkSemaphoreSubmitInfo, 2> waitInfos = { waitInfo1, waitInfo2 };
    std::array<VkSemaphoreSubmitInfo, 1> signalInfos = { signalInfo1 };

    VkSubmitInfo2 submitInfo2{};
    submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo2.commandBufferInfoCount = 1;
    submitInfo2.pCommandBufferInfos = &commandBufferInfo;
    submitInfo2.waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size());
    submitInfo2.pWaitSemaphoreInfos = waitInfos.data();
    submitInfo2.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
    submitInfo2.pSignalSemaphoreInfos = signalInfos.data();
    //vkQueueSubmit2(graphicsQueue, 1, &submitInfo, nullptr);
    
    if (vkQueueSubmit2(graphicsQueue, 1, &submitInfo2, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSemaphore signalSemaphores[] = {renderFinihsedSemaphores[currentFrame]};
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

    //currentCompute = (currentCompute + 1) % MAX_COMPUTE_FRAMES;

    deltaTime = glfwGetTime() - prevTime;
    prevTime = glfwGetTime();
    deltaTimeCount += deltaTime;
    //std::cout << deltaTimeCount;
    

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    

    

    if (deltaTimeCount > FPS_REFRESH_FREQUENCY) {
        if (player->playerOptions & PL_ENABLE_FPS) {
            std::cout << "FPS is " << frameCount / FPS_REFRESH_FREQUENCY <<" | CPS is "<<computeCount / FPS_REFRESH_FREQUENCY << std::endl;
        }
        std::cout << "FPS is " << frameCount / FPS_REFRESH_FREQUENCY << " | CPS is " << computeCount / FPS_REFRESH_FREQUENCY << std::endl;
        frameCount = 0;
        computeCount = 0;
        deltaTimeCount = 0;
    }
    frameCount++;
    //computeCount++;
    
}
void VulkanRenderer::computeFrame() {
    //std::cout << "Compute Waiting: " << currentCompute << std::endl;
    vkWaitForFences(device, 1, &computeFences[currentCompute], VK_TRUE, UINT64_MAX);


        //std::cout << "Auth Transfer | "<<currentCompute<<" | ";

        //std::cout << "Compute wait returned" << std::endl;
        vkResetFences(device, 1, &computeFences[currentCompute]);
        vkResetCommandBuffer(computeCommandBuffers[currentCompute], 0);

        recordComputeCommandBuffer(computeCommandBuffers[currentCompute], currentCompute);

        VkCommandBufferSubmitInfo computeCommandBufferInfo{};
        computeCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        computeCommandBufferInfo.commandBuffer = computeCommandBuffers[currentCompute];

        VkSemaphoreSubmitInfo computeWaitInfo{};
        computeWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        computeWaitInfo.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        computeWaitInfo.semaphore = computeFinishedSemaphores1[currentCompute];

        VkSemaphoreSubmitInfo computeSignalInfo1{};
        computeSignalInfo1.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        computeSignalInfo1.semaphore = computeFinishedSemaphores1[(currentCompute +1) % MAX_COMPUTE_FRAMES];
        computeSignalInfo1.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        //VkSemaphoreSubmitInfo computeSignalInfo2{};
        //computeSignalInfo2.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        //computeSignalInfo2.semaphore = computeFinishedSemaphores2[currentCompute];
        //computeSignalInfo2.stageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        std::array<VkSemaphoreSubmitInfo, 1> computeSignalInfos = { computeSignalInfo1 }; // , computeSignalInfo2
//};

        VkSubmitInfo2 computeSubmitInfo{};
        computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        computeSubmitInfo.commandBufferInfoCount = 1;
        computeSubmitInfo.pCommandBufferInfos = &computeCommandBufferInfo;
        computeSubmitInfo.waitSemaphoreInfoCount = 1;
        computeSubmitInfo.pWaitSemaphoreInfos = &computeWaitInfo;
        computeSubmitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(computeSignalInfos.size());
        computeSubmitInfo.pSignalSemaphoreInfos = computeSignalInfos.data();

        if (FirstFrame) {
            computeSubmitInfo.waitSemaphoreInfoCount = 0;
            computeSubmitInfo.pWaitSemaphoreInfos = nullptr;
            std::cout << "First Frame" << std::endl;
            FirstFrame = false;
        }



        //std::cout << currentCompute << std::endl;
        if (vkQueueSubmit2(computeQueue, 1, &computeSubmitInfo, computeFences[currentCompute]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit compute work");
        }
        
        player->triggerStep = false;

   






    computeCount++;

    //std::cout << "Compute: " << computeCount <<std::flush;


    currentCompute = (currentCompute + 1) % MAX_COMPUTE_FRAMES;

    longComputeCount++;
}


//Validation Layers
std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
    extensions = { winmanager.glfwExtensions, winmanager.glfwExtensions + winmanager.glfwExtensionCount };

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}
void VulkanRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}
bool VulkanRenderer::checkValidationLayerSupport() {
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
VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::cleanupSwapChain() {
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

void VulkanRenderer::cleanup() {
    std::cout << longComputeCount;
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, transferBuffers[i], nullptr);
        vkFreeMemory(device, transferBufferMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, graphicsDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);



    for (size_t i = 0; i < MAX_COMPUTE_FRAMES; i++) {
        vkDestroyBuffer(device, computeBuffers[i], nullptr);
        vkFreeMemory(device, computeBufferMemory[i], nullptr);
        vkDestroySemaphore(device, computeFinishedSemaphores1[i], nullptr);
        vkDestroySemaphore(device, computeFinishedSemaphores2[i], nullptr);
        vkDestroyFence(device, computeFences[i], nullptr);
    }

    

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSempahores[i], nullptr);
        vkDestroySemaphore(device, renderFinihsedSemaphores[i], nullptr);
        vkDestroySemaphore(device, transferFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, vertexFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
        vkDestroyFence(device, transferFinishedFences[i], nullptr);
    }
    
    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
    vkDestroyCommandPool(device, computeCommandPool, nullptr);
    vkDestroyCommandPool(device, transferCommandPool, nullptr);

    vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);


    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipeline(device, computePipeline, nullptr);


    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}