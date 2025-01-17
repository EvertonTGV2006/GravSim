#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <map>
#include <atomic>
#include <vector>

#include "window.h"
#include "structs.h"
#include <chrono>
//#include "renderer.h"
#include "statusLogger.h"

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
	StatusLogger* stat;

	std::atomic_bool windowShouldClose;
	bool validateParticles = false;

	glm::vec3 pos = { 0,-0.3,-1.8 };
	glm::vec3 viewDirection = {0, 0.3, 1.8 };
	glm::vec3 viewFocus = { 0, 0, 0 };
	glm::vec3 viewUp = { 0,0,1 };
	float viewZoom = 1.0f;
	const float zoomMin = 0.01f;
	const float zoomMax = 100.0f;

	WindowManager* winmanager;

	uint32_t playerOptions = PL_VIEW_LOCK_FOCUS | PL_VIEW_LOCK_UP;

	glm::mat4 viewMat;

	bool framebufferResized = false;

	double xpos = 0;
	double ypos = 0;

	float anglez = glm::atan(viewDirection.z / sqrt(viewDirection.x * viewDirection.x + viewDirection.y * viewDirection.y));
	float anglexy = glm::atan(viewDirection.y / viewDirection.x) - glm::pi<float>() * 0.5f;
	float scrollScale = 0.1f;

	float xscale = 0.005f;
	float yscale = 0.005f;

	double currentTime = glfwGetTime();;
	double prevTime = glfwGetTime();
	float deltaTime = 0;

	uint64_t playerMoveFlags;
	glm::vec3 playerVelocityDirection = { 0,0,0 };
	float playerVelocityScale = 2;
	float forwardVelocityScale = 0;
	const float forwardVelocityMax = 0.2f;
	float acrossVelocityScale = 0.0f;
	const float acrossVelocityMax = 0.2f;
	const float accelerationScale = 0.1f;
	const float negAccelerationScale = 0.4f;

	std::atomic_bool timeAccel = false;
	std::atomic_bool timeStep = true;
	std::atomic_bool timePause = true;
	std::atomic_bool triggerStep = false;


	std::map<int, uint64_t> keyBindings = { {GLFW_KEY_UP, PL_MOVE_FORWARD}, {GLFW_KEY_DOWN, PL_MOVE_BACKWARD}, {GLFW_KEY_LEFT, PL_MOVE_LEFT}, {GLFW_KEY_RIGHT, PL_MOVE_RIGHT} };

	int windowxpos = 0;
	int windowypos = 0;

	void* playerTurnStr;
	
	std::vector<char> str1 = { 'H','e','l','l','o',' ','W','o','r','l','d' };
	//std::vector<char> str = { 'A', 'n', 'y', 't', 'h', 'i', 'n', 'g' };
	std::string str2 = "TEST_TEXT";
	std::string fLabel = "Frame Count: ";
	std::string tLabel = "FPS: ";

	void initUIElements(uint32_t*, uint32_t*);

	void updateViewMat();
	void updatePlayerMovement();

	void updateGLFWcallbacks();

	static void framebufferResizeCallback(GLFWwindow*, int, int);
	static void mouseMotionCallback(GLFWwindow*, double, double);
	static void keyCallback(GLFWwindow*, int, int, int, int);
	static void scrollCallback(GLFWwindow*, double, double);
	static void windowCloseCallback(GLFWwindow*);
	static void charCallback(GLFWwindow*, uint32_t);

	std::vector<UIBox> boxes;
	UIText texts[128];
	std::string strings[64];

	std::vector<char> inputString;
	std::atomic_bool commandSubmit;
	bool shiftModifier;

	void initScoreBoxes(std::vector<std::vector<std::string>*>*, std::vector<uint32_t*>*, std::vector<std::string>);
	void destroyScoreBoxes();

	std::vector<std::string> playerNames;
	std::array<char, 8> usrn;
	std::array<char, 8> oppn;
};
