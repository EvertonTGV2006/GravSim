#pragma once

#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include "player.h"

void PlayerObject::updateViewMat() {
	glm::vec3 up;
	if (playerOptions & PL_VIEW_LOCK_UP) {
		up = glm::vec3(0, 0, 1);
	}
	else {
		up = viewUp;
	}
	//if (playerOptions & PL_VIEW_LOCK_FOCUS) {
	//	pos = viewFocus + viewDirection * viewZoom;
	//}
	//else {
	//	viewFocus = pos + viewDirection * viewZoom;
	//}

	//viewMat = glm::lookAt(pos, viewFocus, up);
	viewMat = glm::lookAt(pos, pos + (viewDirection*viewZoom), up);

}

void PlayerObject::updateGLFWcallbacks() {
	glfwSetWindowUserPointer(winmanager.window, this);
	int x, y;
	glfwGetFramebufferSize(winmanager.window, &x, &y);
	xpos = x/2; ypos = y/2;
	glfwSetInputMode(winmanager.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetInputMode(winmanager.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetFramebufferSizeCallback(winmanager.window, framebufferResizeCallback);
	glfwSetCursorPosCallback(winmanager.window, mouseMotionCallback);
	glfwSetCursorPos(winmanager.window, xpos, ypos);
	std::cout << "Setting Callbacks";
	glfwSetKeyCallback(winmanager.window, keyCallback);
	glfwSetScrollCallback(winmanager.window, scrollCallback);
	glfwSetWindowCloseCallback(winmanager.window, windowCloseCallback);

}

void PlayerObject::framebufferResizeCallback(GLFWwindow* window, int width, int height){
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
	}
void PlayerObject::mouseMotionCallback(GLFWwindow* window, double xpos, double ypos) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	//std::cout << "Mouse callback" << std::endl;

	double dx = xpos - app->xpos;
	double dy = ypos - app->ypos;
	app->xpos = xpos;
	app->ypos = ypos;
	if (app->playerOptions & PL_VIEW_INVERT_Y_AXIS) {
		dy *= -1;
	}
	app->anglez += dy * app->yscale;
	app->anglexy += dx * app->xscale;
	if (app->anglez < -glm::half_pi<double>()) {
		app->anglez = -glm::half_pi<double>();
	}
	if (app->anglez > glm::half_pi<double>()) {
		app->anglez=glm::half_pi<double>();
	}
	app->viewDirection.z = glm::sin(app->anglez);
	app->viewDirection.x = glm::sin(app->anglexy) * glm::sqrt(1 - glm::pow(app->viewDirection.z, 2));
	app->viewDirection.y = glm::cos(app->anglexy) * glm::sqrt(1 - glm::pow(app->viewDirection.z, 2));

	//std::cout << "sin + cos" << glm::pow(glm::sin(app->anglexy),2) + glm::cos(app->anglexy) << std::endl;;

	//std::cout << "View Direction: " << app->viewDirection.x << ", " << app->viewDirection.y << ", " << app->viewDirection.z << " | " << app->anglez<< " | "<< glm::length(app->viewDirection)<< std::endl;
	//std::cout << "Angle XY: " << app->anglexy << " | Angle Z: " << app->anglez << std::endl;
	


}
void PlayerObject::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	std::cout << "Key: " << key << " | Action: " << action << std::endl;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		//std::cout << "Window should close cmd";
		app->windowShouldClose = true;
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		//std::cout << "Close Window!" << std::endl;
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (app->timePause == false) { app->timePause = true; }
		else if (app->timePause == true) { app->timePause = false; }
	}
	else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		app->triggerStep = true;
	}
	else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		app->timeAccel = true;
	}
	else if (key == GLFW_KEY_TAB && action == GLFW_RELEASE) {
		app->timeAccel = false;	
	}
	else if (action == GLFW_PRESS && key == GLFW_KEY_P) {
		if (glfwGetWindowMonitor(window) == NULL) {
			glfwGetWindowPos(window, &app->windowxpos, &app->windowypos);
			glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
		}
	}
	else if (action == GLFW_PRESS && key == GLFW_KEY_O) {
		if (glfwGetWindowMonitor(window) != NULL) {
			glfwSetWindowMonitor(window, NULL, app->windowxpos, app->windowypos, 800, 600, GLFW_DONT_CARE);
		}
	}
	else if (action == GLFW_PRESS) {
			app->playerMoveFlags |= app->keyBindings[key];
		}
	else if (action == GLFW_RELEASE){
		app->playerMoveFlags &= ~app->keyBindings[key];
	}
	//std::cout << app->pos.x << " " << app->pos.y << " " << app->pos.z << std::endl;
	//std::cout << app->playerMoveFlags << std::endl;
}
void PlayerObject::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));


	app->viewZoom += yoffset* app->scrollScale * app->viewZoom;
	if (app->viewZoom > app->zoomMax) {
		app->viewZoom = app->zoomMax;
	}	
	else if (app->viewZoom < app->zoomMin) {
		app->viewZoom = app->zoomMin;
	}
	std::cout << app->viewZoom << std::endl;
}
void PlayerObject::updatePlayerMovement() {
	
	currentTime = glfwGetTime();
	deltaTime = currentTime - prevTime;
	prevTime = currentTime;


	if (playerMoveFlags & PL_MOVE_FORWARD && playerMoveFlags & PL_MOVE_BACKWARD || (playerMoveFlags & PL_MOVE_FORWARD)==0 && (playerMoveFlags & PL_MOVE_BACKWARD)==0) {
		if (forwardVelocityScale > deltaTime * negAccelerationScale) {
			forwardVelocityScale -= deltaTime * negAccelerationScale;
			
		}
		else if (-forwardVelocityScale > deltaTime * negAccelerationScale) {
			forwardVelocityScale += deltaTime * negAccelerationScale;
		}
		else { forwardVelocityScale = 0; }
	}

	else if (playerMoveFlags & PL_MOVE_FORWARD) {
		if (forwardVelocityScale < forwardVelocityMax) { forwardVelocityScale += deltaTime * accelerationScale; }
		else { forwardVelocityScale = forwardVelocityMax; }
	}
	else if (playerMoveFlags & PL_MOVE_BACKWARD) {
		if (forwardVelocityScale > -forwardVelocityMax) { forwardVelocityScale -= deltaTime * accelerationScale; }
		else { forwardVelocityScale = -forwardVelocityMax; }
	}

	if (playerMoveFlags & PL_MOVE_LEFT && playerMoveFlags & PL_MOVE_RIGHT || (playerMoveFlags & PL_MOVE_LEFT) == 0 && (playerMoveFlags & PL_MOVE_RIGHT) == 0) {
		if (acrossVelocityScale > deltaTime * accelerationScale) {
			acrossVelocityScale -= deltaTime * accelerationScale;
		}
		else if (-acrossVelocityScale > deltaTime * accelerationScale) {
			acrossVelocityScale += deltaTime * accelerationScale;
		}
		else { acrossVelocityScale = 0; }
	}
	else if (playerMoveFlags & PL_MOVE_RIGHT) {
		if (acrossVelocityScale < acrossVelocityMax) { acrossVelocityScale += deltaTime * accelerationScale; }
		else { acrossVelocityScale = acrossVelocityMax; }
	}
	else if (playerMoveFlags & PL_MOVE_LEFT) {
		if (-acrossVelocityScale < acrossVelocityMax) { acrossVelocityScale -= deltaTime * accelerationScale; }
		else { acrossVelocityScale = -acrossVelocityMax; }
	}
	//std::cout << forwardVelocityScale << " | " << acrossVelocityScale << std::endl;
	playerVelocityDirection = forwardVelocityScale  * viewDirection + acrossVelocityScale * glm::cross(viewDirection, viewUp);
	
	//std::cout << "XYZ: " << playerVelocityDirection.x << ", " << playerVelocityDirection.y << ", " << playerVelocityDirection.z << std::endl;
	//std::cout << glm::length(viewDirection) << "   " << glm::length(viewUp)<<std::endl;
	if (playerVelocityDirection != glm::vec3(0)) {
		playerVelocityDirection = glm::normalize(playerVelocityDirection);

	}



	pos += playerVelocityDirection * (float)deltaTime * playerVelocityScale;


	//std::cout << pos.x << " | " << pos.y << " | " << pos.z << std::endl;
}

void PlayerObject::windowCloseCallback(GLFWwindow* window) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	std::cout << "Window close callback";
	app->windowShouldClose = true;
}