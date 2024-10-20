#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <cstdlib>
#include <iostream>
#include <vector>
#include <optional>
#include <array>
#include <glm/glm.hpp>



#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_BITMAP_H

enum UIConfiguration {
    UI_RENDER_MODE_FLEXIBLE = 1,
    UI_RENDER_MODE_STATIC = 2,
    UI_REFERENCE_MODE_UINT32_T = 4,
    UI_REFERENCE_MODE_STRING = 8,
    UI_REFERENCE_MODE_CHAR = 16
};

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
        features->shaderFloat64 = VK_FALSE;
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

    void print() {
        std::cout << "Position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Velocity: " << velocity.x << ", " << velocity.y << ", " << velocity.z << std::endl;
        std::cout << "Mass: " << mass << " | Cell: " << cell << " | newIndex: " << newIndex << std::endl;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getParticleAttributeDescriptions();
    static VkVertexInputBindingDescription getParticleInputBindings();
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

struct textBitmapWrapper {
    char character;
    int16_t advance;
    int16_t bearingX;
    int16_t bearingY;
    FT_Bitmap* address;
};
struct UIElement {
    glm::vec2 screenPosition;
    glm::vec2 textPosition;
    uint32_t configuration;
    void* dataPointer;
    void getCharVector(std::vector<char>*);
};
