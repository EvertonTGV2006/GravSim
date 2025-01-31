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
	if (!focusOnPlanet) {
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
		up = viewUp;

		//viewMat = glm::lookAt(pos, viewFocus, up);
		viewMat = glm::lookAt(pos, pos + (viewDirection * viewZoom)/*glm::vec3(0.0f,0.0f,0.0f)*/, up);
	}
	else {
		glm::dvec3 plPos = (*planets)[planetIndex].pos_2;
		glm::dvec3 radiusVec = (*planets)[planetIndex].radius * viewZoom * glm::dvec3(1.0f, 1.0f, 1.0f);
		radiusVec.z *= glm::sin(anglez);
		radiusVec.y *= glm::sin(anglexy) * glm::sqrt(1.0f - glm::pow(glm::sin(anglez), 2.0f));
		radiusVec.x *= glm::cos(anglexy) * glm::sqrt(1.0f - glm::pow(glm::sin(anglez), 2.0f));

		viewMat = glm::lookAt(plPos + radiusVec, plPos, glm::dvec3(0.f, 0.0f, 1.0f));
	}

}

void PlayerObject::updateGLFWcallbacks() {
	glfwSetWindowUserPointer(winmanager->window, this);
	int x, y;
	glfwGetFramebufferSize(winmanager->window, &x, &y);
	xpos = x/2; ypos = y/2;
	glfwSetInputMode(winmanager->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	//glfwSetInputMode(winmanager->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetFramebufferSizeCallback(winmanager->window, framebufferResizeCallback);
	glfwSetCursorPosCallback(winmanager->window, mouseMotionCallback);
	glfwSetMouseButtonCallback(winmanager->window, mouseButtonCallback);
	glfwSetCursorPos(winmanager->window, xpos, ypos);
	//std::cout << "Setting Callbacks";
	stat->addMessage(MSG_LEVEL_STARTUP_LOW, "Setting GLFW callbacks");
	glfwSetKeyCallback(winmanager->window, keyCallback);
	glfwSetScrollCallback(winmanager->window, scrollCallback);
	glfwSetWindowCloseCallback(winmanager->window, windowCloseCallback);
	glfwSetCharCallback(winmanager->window, charCallback);

}

void PlayerObject::initUIElements(uint32_t* frameIndex, uint32_t* fpsVal) {

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
	commandBox.scale = glm::vec2(0.9f, 0.9f);
	commandBox.colour = glm::vec3(1.0f, 1.0f, 1.0f);;
	commandBox.dataP = &(texts[4]);
	commandBox.textCount = 1;

	UIBox frameCounterBox{};
	frameCounterBox.pos = glm::vec2(0.02f, 0.02f);
	frameCounterBox.scale = glm::vec2(0.9f, 0.9f);
	frameCounterBox.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	frameCounterBox.dataP = &(texts[0]);
	frameCounterBox.textCount = 4;

	UIBox playerTurnBox = frameCounterBox;
	playerTurnBox.dataP = &texts[126];
	playerTurnBox.textCount = 1;

	UIBox statBox{};
	statBox.pos = glm::vec2(0.5f, 0.02f);
	statBox.scale = glm::vec2(0.5f, 0.8f);
	statBox.colour = glm::vec3(1);
	statBox.dataP = &(stat->texts[0]);
	statBox.textCount = stat->currentMsgCount;


	boxes.push_back(commandBox);
	boxes.push_back(frameCounterBox);
	//boxes.push_back(statBox);

	//inputString.push_back('w');

	UIBox paramBox{};
	paramBox.pos = glm::vec2(0.2f, 0.8f);
	paramBox.scale = glm::vec2(0.75f, 1.0f);
	paramBox.colour = glm::vec3(1.0f, 1.0f, 1.0f);
	paramBox.dataP = &(texts[8]);
	paramBox.textCount = 6;



	UIText paramText{};
	paramText.colour = glm::vec3(0.5, 1, 1);
	paramText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_STRING;
	paramText.dataP = &strings[2];
	strings[2] = "Timestep:";
	texts[8] = paramText;
	paramText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_DOUBLE;
	paramText.dataP = &params.dt;
	texts[9] = paramText;
	paramText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
	paramText.dataP = &strings[3];
	strings[3] = "Elasped Time:";
	texts[10] = paramText;
	paramText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_DOUBLE_SECONDS;
	paramText.dataP = &params.elapsedTime;
	texts[11] = paramText;
	paramText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
	paramText.dataP = &strings[4];
	strings[4] = "Mesh:";
	texts[12] = paramText;
	paramText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_BOOL;
	paramText.dataP = &params.mesh;
	texts[13] = paramText;
	paramText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
	paramText.dataP = &strings[5];
	strings[5] = "Pause:";
	texts[14] = paramText;
	paramText.config = UI_ALIGNMENT_H_R | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_BOOL;
	paramText.dataP = &params.pause;
	texts[15] = paramText;

	boxes.push_back(paramBox);

	
}

void PlayerObject::framebufferResizeCallback(GLFWwindow* window, int width, int height){
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
	}
void PlayerObject::mouseMotionCallback(GLFWwindow* window, double xpos, double ypos) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	//std::cout << "Mouse callback: " << app->xpos << " | "<< app->ypos << std::endl;
	std::lock_guard<std::mutex> lock(app->mouseMutex);

	float dx = float(xpos - app->xpos);
	float dy = float(ypos - app->ypos);
	app->xpos = xpos;
	app->ypos = ypos;
	if (app->playerOptions & PL_VIEW_INVERT_Y_AXIS) {
		dy *= -1;
	}

	if (app->middleMouseButtonPressed) {
		app->anglez += dy * app->yscale;
		app->anglexy += dx * app->xscale;
		if (app->anglez <= -glm::half_pi<float>() + 0.001f) {
			app->anglez = -glm::half_pi<float>() + 0.001f;
		}
		if (app->anglez >= glm::half_pi<float>() - 0.001f) {
			app->anglez = glm::half_pi<float>() - 0.001f;
		}

		app->viewDirection.z = glm::sin(app->anglez);
		app->viewDirection.x = glm::sin(app->anglexy) * glm::sqrt(1.0f - glm::pow(app->viewDirection.z, 2.0f));
		app->viewDirection.y = glm::cos(app->anglexy) * glm::sqrt(1.0f - glm::pow(app->viewDirection.z, 2.0f));
	}

	////std::cout << "sin + cos" << glm::pow(glm::sin(app->anglexy),2) + glm::cos(app->anglexy) << std::endl;;

	////std::cout << "View Direction: " << app->viewDirection.x << ", " << app->viewDirection.y << ", " << app->viewDirection.z << " | " << app->anglez<< " | "<< glm::length(app->viewDirection)<< std::endl;
	////std::cout << "Angle XY: " << app->anglexy << " | Angle Z: " << app->anglez << std::endl;
	//
}
void PlayerObject::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	std::lock_guard<std::mutex> lock(app->mouseMutex);
	if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		if (action == GLFW_PRESS) {
			if (app->stickyMiddleMouseButton) {
				app->middleMouseButtonPressed = !app->middleMouseButtonPressed;
				if (app->middleMouseButtonPressed) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
				}
				else {
					glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
			}
			else {
				app->middleMouseButtonPressed = true;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
			}
		}
		else if (action == GLFW_RELEASE) {
			if (app->stickyMiddleMouseButton) {}
			else {
				app->middleMouseButtonPressed = false;
				glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}
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
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_TAB) {
			if (mods == GLFW_FALSE) {
				app->planetIndex = (app->planetIndex + 1) % MAX_PLANET_ARRAY_SIZE;
			}
			else if (mods == GLFW_MOD_SHIFT) {
				app->planetIndex = (app->planetIndex) ? app->planetIndex - 1 : MAX_PLANET_ARRAY_SIZE - 1;
			}
		}
	}
	
}
void PlayerObject::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	std::lock_guard<std::mutex> lock(app->mouseMutex);

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

void PlayerObject::processCommand() {
	std::string str;
	str.resize(inputString.size());
	memcpy(str.data(), inputString.data(), inputString.size());

	bool success = true;

	if (str.substr(0, 7) == "/toggle") {
		if (str.length() > 8) {
			if (str.substr(8, 2) == "-m") {
				params.mesh = !params.mesh;
			}
			else if (str.substr(8, 2) == "-l") {
				params.satLines = !params.satLines;
			}
			else {
				success = false;
			}
		}
		else {
			success = false;
		}
	}
	else if (str.substr(0, 6) == "/pause") {
		params.pause = !params.pause;
	}
	else if (str.substr(0, 11) == "/fullscreen") {
		params.fullscreen = !params.fullscreen;
		if(params.fullscreen){
			if (glfwGetWindowMonitor(winmanager->window) == NULL) {
				glfwGetWindowPos(winmanager->window, &windowxpos, &windowypos);
				glfwSetWindowMonitor(winmanager->window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
			}
		}
		else {
			if (glfwGetWindowMonitor(winmanager->window) != NULL) {
				glfwSetWindowMonitor(winmanager->window, NULL, windowxpos, windowypos, 800, 600, GLFW_DONT_CARE);
			}
		}
	}
	else if (str.substr(0, 5) == "/time") {
		std::stringstream iss(str.substr(6));
		iss >> params.dt;
		stat->addMessage(MSG_LEVEL_USER, "Set Timestep to " + iss.str());
	}
	else if (str.substr(0, 6) == "/clear") {
		if (str.length() > 7) {
			if (str.substr(7, 2) == "-l") {
				params.clearSatLines = true;
			}
			else {
				success = false;
			}
		}
		else {
			success = false;
		}
	}
	else if (str.substr(0, 5) == "/kill") {
		glfwSetWindowShouldClose(winmanager->window, GLFW_TRUE);
	}
	else {
		success = false;
	}
	if (success) {
		inputString.clear();
	}
	else {
		stat->addMessage(MSG_LEVEL_USER, "Invalid Command");
	}
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



	pos += playerVelocityDirection * (float)deltaTime * playerVelocityScale * 6e7f;


	//std::cout << pos.x << " | " << pos.y << " | " << pos.z << std::endl;
}

void PlayerObject::windowCloseCallback(GLFWwindow* window) {
	auto app = reinterpret_cast<PlayerObject*>(glfwGetWindowUserPointer(window));
	//std::cout << "Window close callback";
	app->stat->addMessage(MSG_LEVEL_DEBUG, "Window Closing");
	app->windowShouldClose = true;
}

