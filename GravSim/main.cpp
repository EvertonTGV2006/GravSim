#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <fstream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>



#include "engine.h"
#include "window.h"
#include "geometry.h"

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif






int main() {
	try {
		PlayerObject player;
		VulkanEngine engine;
		engine.player = &player;
		engine.initEngine();
		engine.startDraw();
		engine.cleanup();

	}
}

