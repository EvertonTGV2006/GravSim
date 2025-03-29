#pragma once

#include <GLFW/glfw3.h>
#include "window.h"

#include "stb_image.h"

void WindowManager::initWindow() {


	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);


	//window = glfwCreateWindow(mode->width, mode->height, "Durak", primary, nullptr); //fullscreen
	window = glfwCreateWindow(800, 600, "Durak", nullptr, nullptr); //windowed

	GLFWimage image[1];
	image[0].pixels = stbi_load("textures/icon.png", &image[0].width, &image[0].height, 0, 4);
	glfwSetWindowIcon(window, 1, image);
	stbi_image_free(image[0].pixels);


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