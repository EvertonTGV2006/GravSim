#pragma once

#include <iostream>
#include <chrono>
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
	glfwSetCharCallback(winmanager.window, charCallback);

}

void PlayerObject::initUIElements(uint32_t* frameIndex, uint32_t* fpsVal) {
	elements.resize(4);
	elements[0].textPosition = glm::vec2(-0.5, -0.5);
	elements[0].textDimension = glm::vec2(10, 0.25);
	elements[0].dataPointer = &str2;
	elements[0].configuration = UI_REFERENCE_MODE_STRING;
	elements[1].textDimension = glm::vec2(10, 0.25);
	elements[1].textPosition = glm::vec2(0, 0);
	elements[1].configuration = UI_REFERENCE_MODE_UINT32_T;
	elements[1].dataPointer = &number;
	elements[2].textDimension = glm::vec2(3, 0.1);
	elements[2].textPosition = glm::vec2(-0.9, -0.9);
	elements[2].dataPointer = frameIndex;
	elements[2].labelPointer = &fLabel;
	elements[2].configuration = UI_REFERENCE_MODE_LABEL_STR_VALUE;
	elements[3].textDimension = glm::vec2(3, 0.1);
	elements[3].textPosition = glm::vec2(-0.2, -0.9);
	elements[3].dataPointer = fpsVal;
	elements[3].labelPointer = &tLabel;
	elements[3].configuration = UI_REFERENCE_MODE_LABEL_STR_VALUE;

	std::string frameString1 = "Frame Count: ";
	strings[0] = frameString1;
	frameString1 = "FPS: ";
	strings[1] = frameString1;

	UIText frameCounterText1{};
	frameCounterText1.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_STRING;
	frameCounterText1.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	frameCounterText1.dataP = &(strings[0]);
	texts[0] = frameCounterText1;

	frameCounterText1.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_UINT32_T;
	frameCounterText1.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	frameCounterText1.dataP = frameIndex;
	texts[1] = frameCounterText1;

	frameCounterText1.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
	frameCounterText1.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	frameCounterText1.dataP = &(strings[1]);
	texts[2] = frameCounterText1;

	frameCounterText1.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_UINT32_T;
	frameCounterText1.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	frameCounterText1.dataP = fpsVal;
	texts[3] = frameCounterText1;

	UIText commandText{};
	commandText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_B | UI_NEWLINE_FALSE | UI_DATA_CHAR_VEC;
	commandText.colour = glm::vec3(0.9f, 0.9f, 0.92f);
	commandText.dataP = &inputString;
	texts[4] = commandText;

	UIBox commandBox{};
	commandBox.pos = glm::vec2(0.02f, 0.02f);
	commandBox.size = glm::vec2(0.9f, 0.9f);
	commandBox.colour = glm::vec3(1.0f, 1.0f, 1.0f);;
	commandBox.dataP = &(texts[4]);
	commandBox.textCount = 1;

	UIBox frameCounterBox{};
	frameCounterBox.pos = glm::vec2(0.02f, 0.02f);
	frameCounterBox.size = glm::vec2(0.9f, 0.9f);
	frameCounterBox.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	frameCounterBox.dataP = &(texts[0]);
	frameCounterBox.textCount = 4;

	boxes.push_back(commandBox);
	boxes.push_back(frameCounterBox);

	//inputString.push_back('w');
	

	
}
void PlayerObject::initScoreBoxes(std::vector<std::vector<std::string>*>* playerScoreReasons, std::vector<uint32_t*>* playerScores, std::vector<std::string>pNames) {
	playerNames = pNames;

	uint32_t textZero = 5;
	uint32_t minScore = static_cast<uint32_t>(std::min((*playerScoreReasons)[0]->size(), (*playerScoreReasons)[1]->size()));
	uint32_t maxScore = static_cast<uint32_t>(std::max((*playerScoreReasons)[0]->size(), (*playerScoreReasons)[1]->size()));
	uint32_t maxIndex = ((*playerScoreReasons)[0]->size() > (*playerScoreReasons)[1]->size()) ? 0 : 1;
	std::cout << maxIndex << std::endl;

	UIText commandText{};
	commandText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_STRING;
	commandText.colour = glm::vec3(0.9f, 0.9f, 0.92f);
	commandText.dataP = &playerNames[0];
	texts[textZero] = commandText;
	commandText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_STRING;
	commandText.dataP = &playerNames[1];
	texts[textZero + 1] = commandText;


	for (uint32_t i = 0; i < minScore; i++) {
		commandText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
		commandText.dataP = &(*(*playerScoreReasons)[0])[i];
		texts[textZero + 2 * i + 2] = commandText;
		commandText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_STRING;
		commandText.dataP = &(*(*playerScoreReasons)[1])[i];
		texts[textZero + 2 * i + 3] = commandText;
	}
	for (uint32_t i = minScore; i < maxScore; i++) {
		if (maxIndex == 0) {
			commandText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
			commandText.dataP = &(*(*playerScoreReasons)[0])[i];
		}
		else if (maxIndex == 1) {
			commandText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
			commandText.dataP = &(*(*playerScoreReasons)[1])[i];
		}


		texts[textZero + 2 + minScore + i] = commandText;
	}
	UIBox commandBox{};
	commandBox.pos = glm::vec2(0.02f, 0.02f);
	commandBox.size = glm::vec2(0.9f, 0.9f);
	commandBox.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	commandBox.dataP = &(texts[textZero]);
	commandBox.textCount = 2 + minScore + maxScore;
	boxes.push_back(commandBox);
	
}
void PlayerObject::destroyScoreBoxes() {
	boxes.pop_back();
}

void PlayerObject::framebufferResizeCallback(GLFWwindow* window, int width, int height){
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
	}
void PlayerObject::mouseMotionCallback(GLFWwindow* window, double xpos, double ypos) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	//std::cout << "Mouse callback" << std::endl;

	float dx = float(xpos - app->xpos);
	float dy = float(ypos - app->ypos);
	app->xpos = xpos;
	app->ypos = ypos;
	if (app->playerOptions & PL_VIEW_INVERT_Y_AXIS) {
		dy *= -1;
	}
	app->anglez += dy * app->yscale;
	app->anglexy += dx * app->xscale;
	if (app->anglez <= -glm::half_pi<float>()+0.001f) {
		app->anglez = -glm::half_pi<float>() + 0.001f;
	}
	if (app->anglez >= glm::half_pi<float>()-0.001f) {
		app->anglez=glm::half_pi<float>() - 0.001f;
	}
	app->viewDirection.z = glm::sin(app->anglez);
	app->viewDirection.x = glm::sin(app->anglexy) * glm::sqrt(1.0f - glm::pow(app->viewDirection.z, 2.0f));
	app->viewDirection.y = glm::cos(app->anglexy) * glm::sqrt(1.0f - glm::pow(app->viewDirection.z, 2.0f));

	//std::cout << "sin + cos" << glm::pow(glm::sin(app->anglexy),2) + glm::cos(app->anglexy) << std::endl;;

	//std::cout << "View Direction: " << app->viewDirection.x << ", " << app->viewDirection.y << ", " << app->viewDirection.z << " | " << app->anglez<< " | "<< glm::length(app->viewDirection)<< std::endl;
	//std::cout << "Angle XY: " << app->anglexy << " | Angle Z: " << app->anglez << std::endl;
	


}
void PlayerObject::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	//std::cout << "Key: " << key << " | Action: " << action << std::endl;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		//std::cout << "Window should close cmd";
		app->windowShouldClose = true;
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		//std::cout << "Close Window!" << std::endl;
	}
	//else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
	//	if (app->timePause == false) { app->timePause = true; }
	//	else if (app->timePause == true) { app->timePause = false; }
	//}
	//else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
	//	app->triggerStep = true;
	//}
	//else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
	//	app->timeAccel = true;
	//}
	//else if (key == GLFW_KEY_TAB && action == GLFW_RELEASE) {
	//	app->timeAccel = false;	
	//}
	//else if (key == GLFW_KEY_V && action == GLFW_PRESS) {
	//	app->validateParticles = true; //validate particles
	//}
	//else if (action == GLFW_PRESS && key == GLFW_KEY_P) {
	//	if (glfwGetWindowMonitor(window) == NULL) {
	//		glfwGetWindowPos(window, &app->windowxpos, &app->windowypos);
	//		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
	//	}
	//}
	//else if (action == GLFW_PRESS && key == GLFW_KEY_O) {
	//	if (glfwGetWindowMonitor(window) != NULL) {
	//		glfwSetWindowMonitor(window, NULL, app->windowxpos, app->windowypos, 800, 600, GLFW_DONT_CARE);
	//	}
	//}
	//else if (action == GLFW_PRESS && key == GLFW_KEY_Z) {
	//	app->triggerStep = true;
	//}
	else if (action == GLFW_PRESS) {
			app->playerMoveFlags |= app->keyBindings[key];
		}
	else if (action == GLFW_RELEASE){
		app->playerMoveFlags &= ~app->keyBindings[key];
	}
	//std::cout << app->pos.x << " " << app->pos.y << " " << app->pos.z << std::endl;
	//std::cout << app->playerMoveFlags << std::endl;
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
			app->commandSubmit = true;
		}
		else if (key == GLFW_KEY_BACKSPACE) {
			if (app->inputString.size() > 0) {
				app->inputString.pop_back();
			}
		}
	}
}
void PlayerObject::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));


	app->viewZoom += float(yoffset)* app->scrollScale * app->viewZoom;
	if (app->viewZoom > app->zoomMax) {
		app->viewZoom = app->zoomMax;
	}	
	else if (app->viewZoom < app->zoomMin) {
		app->viewZoom = app->zoomMin;
	}
	//std::cout << app->viewZoom << std::endl;
}
void PlayerObject::charCallback(GLFWwindow* window, uint32_t code) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	app->inputString.push_back(code);
}
void PlayerObject::updatePlayerMovement() {
	
	currentTime = glfwGetTime();
	deltaTime = float(currentTime - prevTime);
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