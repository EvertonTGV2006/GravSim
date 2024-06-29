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
		uint32_t cID = 63;
		glm::uvec3 GRID_DIMENSIONS = { 4, 4, 4 };
		glm::uvec3 cellPos;
		cellPos.x = cID % GRID_DIMENSIONS.x;
		cellPos.y = ((cID - cellPos.x) / GRID_DIMENSIONS.x) % GRID_DIMENSIONS.y;
		cellPos.z = ((((cID - cellPos.x) / GRID_DIMENSIONS.x) - cellPos.y) / GRID_DIMENSIONS.y) % GRID_DIMENSIONS.z;
		std::cout << cellPos.x << " " << cellPos.y << " " << cellPos.z << std::endl;



		PlayerObject player;
		VulkanEngine engine;
		engine.player = &player;
		engine.initEngine();
		engine.startDraw();
		engine.cleanup();

	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}

