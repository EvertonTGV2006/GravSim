#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "structs.h"

#include <cstdlib>
#include <iostream>
#include <vector>
#include <optional>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <charconv>





VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, colour);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}
void Vertex::printVertex() {
    std::cout << "Position: " << pos.x << ", " << pos.y << ", " << pos.z << " Colour: " << colour.r << ", " << colour.g << ", " << colour.b << std::endl;
}

VkVertexInputBindingDescription Particle::getParticleInputBindings() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, 4> Particle::getParticleAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Particle, velocity);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[2].offset = offsetof(Particle, cell);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[3].offset = offsetof(Particle, newIndex);

    return attributeDescriptions;
}

glm::mat4 Planet::getModelMatrix(float radiusScale) {
    glm::mat4 scale = glm::mat4(radius * radiusScale);
    scale[3][3] = 1.0f;
    glm::mat4 rot = glm::rotate(theta, axis);
    glm::mat4 posMat = glm::translate(pos);
    return posMat * rot * scale;
}

VkVertexInputBindingDescription LineVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(LineVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, 4> LineVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(LineVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(LineVertex, baseColour);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(LineVertex, intColour);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(LineVertex, finColour);

    return attributeDescriptions;
}

void UIElement::getCharVector(std::vector<char>* data, std::vector<uint32_t>* charCounts) {
    uint32_t REFERENCE_CONFIGURATIONS = 4 | 8 | 16 | 32;
    uint32_t referenceConfig = configuration & REFERENCE_CONFIGURATIONS;
    std::vector<char>* charPointer;
    uint32_t endLimit = 0;
    if (UI_REFERENCE_MODE_UINT32_T & referenceConfig) {
        data->resize(10 + data->size());
        std::to_chars(data->data() + data->size() - 10, data->data() + data->size(), *reinterpret_cast<uint32_t*>(dataPointer));
        endLimit = 0;
        for (uint32_t i = 0; i < 10; i++) {
            if ((*data)[data->size() - 1 - i] != 0) {
                endLimit = i;
                break;
            }
        }
        data->resize(data->size() - endLimit);
        charCounts->push_back(10 - endLimit);
    }
    else if (UI_REFERENCE_MODE_CHAR & referenceConfig) {
        charPointer = reinterpret_cast<std::vector<char>*>(dataPointer);
        data->resize(data->size() + charPointer->size());
        std::memcpy(data->data() + data->size() - charPointer->size(), charPointer->data(), charPointer->size());
        charCounts->push_back(static_cast<uint32_t>(charPointer->size()));
    }
    else if (UI_REFERENCE_MODE_STRING & referenceConfig) {
        std::string* str = reinterpret_cast<std::string*>(dataPointer);
        for (uint32_t i = 0; i < str->size(); i++) {
            data->push_back((*str)[i]);
        }
        charCounts->push_back(static_cast<uint32_t>(str->size()));
    }
    else if (UI_REFERENCE_MODE_LABEL_CHAR_VALUE & referenceConfig) {
        charPointer = reinterpret_cast<std::vector<char>*>(labelPointer);
        data->resize(data->size() + charPointer->size());
        std::memcpy(data->data() + data->size() - charPointer->size(), charPointer->data(), charPointer->size());


        data->resize(10 + data->size());
        std::to_chars(data->data() + data->size() - 10, data->data() + data->size(), *reinterpret_cast<uint32_t*>(dataPointer));
        endLimit = 0;
        for (uint32_t i = 0; i < 10; i++) {
            if ((*data)[data->size() - 1 - i] != 0) {
                endLimit = i;
                break;
            }
        }
        data->resize(data->size() - endLimit);
        charCounts->push_back(endLimit + static_cast<uint32_t>(charPointer->size()));
    }
    else if (UI_REFERENCE_MODE_LABEL_STR_VALUE & referenceConfig) {
        std::string* str = reinterpret_cast<std::string*>(labelPointer);
        for (uint32_t i = 0; i < str->size(); i++) {
            data->push_back((*str)[i]);
        }

        data->resize(10 + data->size());
        std::to_chars(data->data() + data->size() - 10, data->data() + data->size(), *reinterpret_cast<uint32_t*>(dataPointer));
        endLimit = 0;
        uint32_t tVal = 0;
        for (uint32_t i = 0; i < 10; i++) {
            tVal = (*data)[data->size() - 1 - i];
            if (tVal != 0) {
                endLimit = i;
                break;
            }
        }
        data->resize(data->size() - endLimit);
        charCounts->push_back(10 - endLimit + static_cast<uint32_t>(str->size()));
    }
}
    