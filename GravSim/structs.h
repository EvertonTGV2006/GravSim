#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <cstdlib>
#include <iostream>
#include <vector>
#include <optional>
#include <array>
#include <glm/glm.hpp>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
struct Vertex {
    glm::vec3 pos;
    glm::vec3 colour;
    glm::vec2 texCoord;
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
    void printVertex();
};
struct Edge {
    uint16_t vert0;
    uint16_t vert1;
};
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 zeta;
};
struct OptionalSettings {
    bool Anisotropy;
    void configureDeviceFeatures(VkPhysicalDeviceFeatures* features) {
        if (Anisotropy) {
            features->samplerAnisotropy = VK_TRUE;
        }
        features->shaderFloat64 = VK_TRUE;
    }
};
struct LightingPushConstants {
    glm::vec3 lightPos;
    glm::vec3 lightColour;
};
struct CameraPushConstants {
    
    glm::vec4 cameraPos;
    glm::vec4 viewDirection;
    glm::vec4 lightPos;
    glm::vec4 lightColour;
   
};
struct ModelPushConstants {
    glm::mat4 modelPos;
    uint32_t mode;
};
struct GlobalPushConstants {
    glm::vec3 lightPos;
    glm::vec3 lightColour;
};
struct Particle {
    /*standard:
    alignas(32) glm::dvec3 position;
    alignas(32) glm::dvec3 velocity;
    */
    /* float 64:
    alignas(32) glm::dvec3 position;
    alignas(32) glm::dvec3 velocity;
    double mass;
    */
    alignas(16)glm::vec3 position;
    alignas(16)glm::vec3 velocity;
    float mass;
    uint32_t cell;
    uint32_t newIndex;


};

struct ComputeConstants {
    double deltaTime;
};

struct MemInit {
    VkDeviceMemory memory;
    uint32_t offset;
    uint32_t range;
};

struct Mesh {
    std::vector<Vertex>* vertices;
    std::vector<uint16_t>* indices;
    uint32_t vertexCount;
    uint32_t indexCount;
};
struct MemoryDetails {
    VkMemoryRequirements requirements;
    VkMemoryPropertyFlags flags;
};