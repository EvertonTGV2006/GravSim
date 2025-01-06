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
    UI_REFERENCE_MODE_CHAR = 16,
    UI_REFERENCE_MODE_LABEL_STR_VALUE = 32,
    UI_REFERENCE_MODE_LABEL_CHAR_VALUE = 64,
    UI_REFERENCE_MODE_LABEL_TIME = 128
};
enum UIOptions {
    //32Bits
    //xxxxxxxxxxxxxxxxxxxxxxnnddddvvhh

    //h = alignment horizontal
    UI_ALIGNMENT_H_MASK = 3,
    UI_ALIGNMENT_H_L = 1,
    UI_ALIGNMENT_H_C = 2,
    UI_ALIGNMENT_H_R = 3,

    //v = alignment vertical
    UI_ALIGNMENT_V_MASK = 3 << 2,
    UI_ALIGNMENT_V_T = 1 << 2,
    UI_ALIGNMENT_V_C = 2 << 2,
    UI_ALIGNMENT_V_B = 3 << 2,

    //d = dataType
    UI_DATA_MASK = 15 << 4,
    UI_DATA_UINT32_T = 1 << 4,
    UI_DATA_FLOAT = 2 << 4,
    UI_DATA_CHAR_VEC = 3 << 4,
    UI_DATA_STRING = 4 << 4,

    //n
    UI_NEWLINE_MASK = 3 << 8,
    UI_NEWLINE_TRUE = 1 << 8,
    UI_NEWLINE_FALSE = 2 << 8
};

const uint32_t LINE_VERTEX_COUNT = 1024;
const uint32_t SATELLITE_COUNT = 64;
const uint32_t MAX_PLANET_ARRAY_SIZE = 2;


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
struct LineVertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 baseColour;
    alignas(16) glm::vec3 intColour;
    alignas(16) glm::vec3 finColour;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
    std::array<glm::mat4, MAX_PLANET_ARRAY_SIZE> models;
    //alignas(256 * 4 - 640) float filler;
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
struct Planet {
    glm::vec3 pos;
    float mass;
    glm::vec3 vel;
    float radius;
    glm::vec3 axis;
    float theta;
    glm::vec4 unused;
    glm::mat4 padding;

    glm::mat4 getModelMatrix(float);
};
struct Satellite {
    glm::vec3 pos;
    float mass;
    glm::vec3 vel;
    float unused;
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
    glm::vec2 textPosition;
    glm::vec2 textDimension;
    uint32_t configuration;
    void* dataPointer;
    void* labelPointer;
    uint32_t* fPointer;
    void getCharVector(std::vector<char>*, std::vector<uint32_t>*);
};



struct UIText {
    uint32_t config;
    uint32_t charCount;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec3 colour;
    void* dataP;
};
struct UIBox {
    uint32_t config;
    uint32_t textCount;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec3 colour;
    UIText* dataP;
};

struct SatExternalMembers {
    VkBuffer* lineBuffer;
    uint32_t* lineCursor;
    uint32_t* lineSegments;
};