#pragma once

#include <GLFW/glfw3.h>
#include <cstdlib>

class WindowManager {
public:

	GLFWwindow *window = 0;
	uint32_t WIDTH = 800;
	uint32_t HEIGHT = 600;
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = 0;

	void cleanup();
	void initWindow();
	void fullscreenWindow(bool);
};