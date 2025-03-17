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
    UI_DATA_DOUBLE = 5 << 4,
    UI_DATA_BOOL = 6 << 4,
    UI_DATA_DOUBLE_SECONDS = 7 << 4,

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
struct MemInit {
    VkDeviceMemory memory;
    uint32_t offset;
    uint32_t range;
};

struct MemoryDetails {
    VkMemoryRequirements requirements;
    VkMemoryPropertyFlags flags;
};

struct UIText {
    uint32_t config;
    float scale;
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
struct UIAttr {
    char c;
    float advx;
    float bx;
    float by;
    float dimx;
    float dimy;
};
struct UIChar {
    int c;
    float scale;
    glm::vec2 sP;
    glm::vec2 sD;
    double padding;
};

struct PlayerDetails {
    char gameID;
    char playerID;
    std::array<char, 16> name;
    char local;
    char unused;
};
struct GlobalParamaters {
    std::array<char, 16> playerName;
    float animationSpeed = 2.0f;
    bool animationPlaying = false;
    int windowWidth;
    int windowHeight;
};