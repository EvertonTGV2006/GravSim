#pragma once

#include <GLFW/glfw3.h>
#include "window.h"

void WindowManager::initWindow() {


	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
}

void WindowManager::cleanup() {
	glfwDestroyWindow(window);
	glfwTerminate();
}
void WindowManager::fullscreenWindow(bool toggle) {
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	glfwSetWindowMonitor(window, monitor, 0, 0, 1920, 1080, GLFW_DONT_CARE);
}