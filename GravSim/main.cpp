#pragma once

#define NOMINMAX

#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <fstream>



#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


#include "cardEngine.h"
#include "engine.h"
#include "window.h"

#include "networking.h"
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

	try {
		if (config["mode"] == "server") {

			NetworkingServer ns;
			ns.stat = &stat;
			ns.initWinsock();
			
		}
		else if (config["mode"] == "host") {
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

			PlayerObject player;
			player.stat = &stat;
			VulkanEngine engine;
			engine.stat = &stat;
			//NetworkingClient nc;
			//std::optional<std::string> usrStr = config["usrn"].value<std::string>();
			//std::array<char, 8> usrn{};
			//for (size_t i = 0; i < usrStr.value().size(); i++) {
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
			engine.player = &player;
			engine.runNumber = 0;
			engine.onlineGame = true;
			engine.initNetworking();
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

			CardEngine cards;
			//cards.setupGame();
			//cards.processTurns();

			PlayerObject player;
			player.stat = &stat;
			VulkanEngine engine;
			engine.stat = &stat;
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
		stat.writeOutMessages();
		std::cerr << e.what() << std::endl;
		std::cout << "Press ENTER to continue...";
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		return EXIT_FAILURE;
	}
	stat.writeOutMessages();
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
