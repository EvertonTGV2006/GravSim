#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <fstream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>



#include "renderer.h"
#include "window.h"
#include "geometry.h"

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void MainLoop(WindowManager* WinMan, VulkanRenderer* renderer) {
	while (!glfwWindowShouldClose(WinMan->window)) {
		glfwPollEvents();
		renderer->queue();
		renderer->computeFrame();
		renderer->drawFrame();
	}
	//renderer.cleanup();
}


void readFile(const std::string& filename, std::vector<char>& filedata) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to Open File");
	}
	size_t fileSize = (size_t)file.tellg();
	filedata.resize(fileSize);
	file.seekg(0);
	file.read(filedata.data(), fileSize);
	file.close();
}



int main() {



	try {
		VulkanRenderer renderer;
		//std::cout << "Enter mode: "; std::cin >> renderer.mode;
		renderer.mode = 0;
		WindowManager winmanager;
		winmanager.initWindow();
		PlayerObject player;
		
		

		player.winmanager = winmanager;
		renderer.winmanager = winmanager;
		renderer.player = &player;
		renderer.threadingEnabled = true;
		renderer.enableValidationLayers = enableValidationLayers;
		player.updateGLFWcallbacks();



		
		double startTime = glfwGetTime();
		readFile("shaders/vert.spv", renderer.vertShaderCode);
		readFile("shaders/frag.spv", renderer.fragShaderCode);
		readFile("shaders/comp.spv", renderer.computeShaderCode);
		renderer.initVulkan();
		double endTime = glfwGetTime();
		std::cout << "Start: " << startTime << " End: " << endTime << " Elapsed Time: " << endTime - startTime << std::endl;
		renderer.queue();
		//MainLoop(&winmanager, &renderer);
		renderer.cleanup();
		winmanager.cleanup();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

