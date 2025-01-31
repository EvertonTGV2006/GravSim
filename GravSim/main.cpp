#pragma once

#define NOMINMAX

#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <fstream>



#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>



#include "engine.h"
#include "window.h"
#include "geometry.h"


#include "statusLogger.h"

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


#include "toml.hpp"


std::string filename = "profiles/default.toml";

auto config = toml::parse_file(filename);





int main() {
	std::ofstream file;
	//file.open("data.csv");
	//file << "\n";
	//file.close();

	StatusLogger stat{};
	PlayerObject player;
	VulkanEngine engine;

	try {

		if (config["mode"] == "host") {
			//uint32_t cID = 63;
			//glm::uvec3 GRID_DIMENSIONS = { 4, 4, 4 };
			//glm::uvec3 cellPos;
			//cellPos.x = cID % GRID_DIMENSIONS.x;
			//cellPos.y = ((cID - cellPos.x) / GRID_DIMENSIONS.x) % GRID_DIMENSIONS.y;
			//cellPos.z = ((((cID - cellPos.x) / GRID_DIMENSIONS.x) - cellPos.y) / GRID_DIMENSIONS.y) % GRID_DIMENSIONS.z;
			//std::cout << cellPos.x << " " << cellPos.y << " " << cellPos.z << std::endl;

			//CardEngine cards;
			//cards.setupGame();
			//cards.processTurns();



			player.stat = &stat;
			engine.stat = &stat;
			//NetworkingClient nc;
			//std::optional<std::string> usrStr = config["usrn"].value<std::string>();
			//std::array<char, 8> usrn{};
			//for (size_t i = 0; i < usrStr.value().scale(); i++) {
			//	usrn[i] = usrStr.value()[i];
			//}
			//nc.usrn = usrn;
			//nc.versionMajor = 1;
			//nc.versionMinor = 0;

			//nc.initWinsock(); /*we have now connected to the server and have an opponent*/

			//player.usrn = nc.usrn;
			//player.oppn = nc.oppn;

			int64_t targetFPS = config["capFPS"].value_or(60);
			if(targetFPS == 0) {
				engine.unlimitedFPS = true;
			}
			else {
				engine.targetFrameTime_uS = static_cast<int>(1000000 / targetFPS);
			}

			engine.unlimitedFPS = true;

			engine.player = &player;
			engine.runNumber = 0;
			engine.onlineGame = true;
			engine.initEngine();
			engine.startDraw();
			engine.cleanup();

		}
		else if (config["mode"] == "offline") {
			//uint32_t cID = 63;
	//glm::uvec3 GRID_DIMENSIONS = { 4, 4, 4 };
	//glm::uvec3 cellPos;
	//cellPos.x = cID % GRID_DIMENSIONS.x;
	//cellPos.y = ((cID - cellPos.x) / GRID_DIMENSIONS.x) % GRID_DIMENSIONS.y;
	//cellPos.z = ((((cID - cellPos.x) / GRID_DIMENSIONS.x) - cellPos.y) / GRID_DIMENSIONS.y) % GRID_DIMENSIONS.z;
	//std::cout << cellPos.x << " " << cellPos.y << " " << cellPos.z << std::endl;
			//cards.setupGame();
			//cards.processTurns();



			int64_t targetFPS = config["targetFPS"].value_or(60);
			if (targetFPS == 0) {
				engine.unlimitedFPS = true;
			}
			else {
				engine.targetFrameTime_uS = static_cast<int>(1000000 / targetFPS);
			}
			engine.player = &player;
			engine.runNumber = 0;
			engine.onlineGame = false;
			engine.initEngine();
			engine.startDraw();
			engine.cleanup();
		}
	}
	catch (const std::exception& e) {
		
		stat.addMessage(MSG_LEVEL_URGENT, e.what());
		player.windowShouldClose = true;
		stat.writeOutMessages();
		std::cerr << e.what() << std::endl;
		std::cout << "Press ENTER to continue...";
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		return EXIT_FAILURE;
	}
	stat.writeOutMessages();
	//std::cout << 1 << std::endl;
	std::cout << "Press ENTER to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	return EXIT_SUCCESS;

}

//int main() {
//	std::ofstream file;
//
//	try {
//		NetworkingHost nh;
//
//		nh.initWinsock();
//	}
//	catch (const std::exception& e) {
//		std::cerr << e.what() << std::endl;
//		file.open("cmd_dmp2.txt");
//		file << e.what() << std::endl;
//		file.close();
//
//
//
//		return EXIT_FAILURE;
//	}
//}
