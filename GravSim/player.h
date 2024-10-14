#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <map>
#include <atomic>

#include "window.h"
//#include "renderer.h"

enum playerViewOptions {
	PL_VIEW_LOCK_FOCUS = 1,
	PL_VIEW_LOCK_UP = 2,
	PL_VIEW_INVERT_Y_AXIS = 4,
	PL_ENABLE_FPS = 2147483648
};
enum playerMoveOptions {
	PL_MOVE_FORWARD = 1,
	PL_MOVE_BACKWARD = 2,
	PL_MOVE_LEFT = 4,
	PL_MOVE_RIGHT = 8
};

class PlayerObject {
public:


	std::atomic_bool windowShouldClose;
	bool validateParticles = false;

	glm::vec3 pos = { -4,27,0 };
	glm::vec3 viewDirection = {4, -27, 0 };
	glm::vec3 viewFocus = { 0, 0, 0 };
	glm::vec3 viewUp = { 0,0,1 };
	float viewZoom = 1;
	const float zoomMin = 0.01;
	const float zoomMax = 100;

	WindowManager winmanager;

	uint32_t playerOptions = PL_VIEW_LOCK_FOCUS | PL_VIEW_LOCK_UP;

	glm::mat4 viewMat;

	bool framebufferResized = false;

	double xpos = 0;
	double ypos = 0;

	double anglez = glm::asin(-1 / sqrt(3));
	double anglexy = glm::pi<double>() * 5 / 4;
	double scrollScale = 0.1;

	float xscale = 0.005;
	float yscale = 0.005;

	double currentTime = glfwGetTime();;
	double prevTime = glfwGetTime();
	double deltaTime = 0;

	uint64_t playerMoveFlags;
	glm::vec3 playerVelocityDirection = { 0,0,0 };
	float playerVelocityScale = 2;
	float forwardVelocityScale = 0;
	const float forwardVelocityMax = 0.2;
	float acrossVelocityScale = 0;
	const float acrossVelocityMax = 0.2;
	const float accelerationScale = 0.1;
	const float negAccelerationScale = 0.4;

	std::atomic_bool timeAccel = false;
	std::atomic_bool timeStep = true;
	std::atomic_bool timePause = true;
	std::atomic_bool triggerStep = false;


	std::map<int, uint64_t> keyBindings = { {GLFW_KEY_W, PL_MOVE_FORWARD}, {GLFW_KEY_S, PL_MOVE_BACKWARD}, {GLFW_KEY_A, PL_MOVE_LEFT}, {GLFW_KEY_D, PL_MOVE_RIGHT} };

	int windowxpos = 0;
	int windowypos = 0;


	void updateViewMat();
	void updatePlayerMovement();

	void updateGLFWcallbacks();

	static void framebufferResizeCallback(GLFWwindow*, int, int);
	static void mouseMotionCallback(GLFWwindow*, double, double);
	static void keyCallback(GLFWwindow*, int, int, int, int);
	static void scrollCallback(GLFWwindow*, double, double);
	static void windowCloseCallback(GLFWwindow*);
};
