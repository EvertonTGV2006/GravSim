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

const double CONSTANT_G = 6.67e-11;
const uint32_t LINE_VERTEX_COUNT = 1024;
const uint32_t SATELLITE_COUNT = 4096;
const uint32_t MAX_PLANET_ARRAY_SIZE = 2;
const uint32_t COMPUTE_STEPS_PER_FRAME = 128; // CANNOT BE 1
const uint32_t SATELLITES_PER_SHADER = 4; 
const uint32_t STEPS_PER_SHADER = 1;
const uint32_t fieldMeshResMajor = 4096;
const uint32_t fieldMeshResMinor = 1;
const uint32_t MESH_PER_SHADER = 4; //aim to make fieldmeshmajor / mesh pershader 1024


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
    UI_DATA_DOUBLE = 5 << 4,
    UI_DATA_BOOL = 6 << 4,

    //n
    UI_NEWLINE_MASK = 3 << 8,
    UI_NEWLINE_TRUE = 1 << 8,
    UI_NEWLINE_FALSE = 2 << 8
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
struct LineVertex {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 baseColour;
    alignas(16) glm::vec3 intColour;
    alignas(16) glm::vec3 finColour;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};
struct LineInfo {
    float eccentricity;
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

    void print() {
        std::cout << "Position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        std::cout << "Velocity: " << velocity.x << ", " << velocity.y << ", " << velocity.z << std::endl;
        std::cout << "Mass: " << mass << " | Cell: " << cell << " | newIndex: " << newIndex << std::endl;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getParticleAttributeDescriptions();
    static VkVertexInputBindingDescription getParticleInputBindings();
};
struct Planet {//FIX THIS TO WORK WITH SHADERS --TO DO--
    alignas(32) glm::dvec3 pos_0;
    alignas(32) glm::dvec3 pos_1;
    alignas(32) glm::dvec3 pos_2;
    double mass;
    alignas(32) glm::dvec3 vel_0;
    alignas(32) glm::dvec3 vel_1;
    alignas(32) glm::dvec3 vel_2;
    double radius;
    glm::dvec3 axis;
    double theta;
    glm::vec4 unused;

    glm::mat4 getModelMatrix(float);
};
struct Satellite {
    glm::dvec3 pos;
    double mass;
    glm::dvec3 vel;
    double unused;
};
struct SatInfo {
    glm::dvec3 pos;
    double mass;
    glm::dvec3 vel;
    double unused;
    double relDistance;
    double relVel;
    double tApproach;
    double padding;
};
struct Orbit {
    uint32_t planetIndex;
    double eccentricity;
    double semiMajorAxis;
    double inclination;
    double ascNodeLong;
    double argPeriapsis;
    double trueAnomaly;

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
    glm::vec2 dim;
    glm::vec2 scale;
    glm::vec3 colour;
    UIText* dataP;
};

struct SatExternalMembers {
    VkBuffer* lineBuffer;
    uint32_t* lineCursor;
    uint32_t* lineSegments;
    VkBuffer* lineInfoBuffer;
    VkBuffer* meshBuffer;
    VkBuffer* meshIndexBuffer;
};
struct GlobalParameters {
    double dt;
    bool mesh;
    bool meshLine;
    bool fullscreen;
    bool pause;
    bool clearSatLines;
    bool satLines;
};