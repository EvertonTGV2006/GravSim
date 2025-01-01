#pragma once
#include <cstdlib>
#include <string>
#include <vector>
#include "structs.h"

struct StatusMessage {
	uint32_t level;
	std::string message;
};
enum MessageFlags {
	MSG_LEVEL_URGENT = 1,
	MSG_LEVEL_USER = 2,
	MSG_LEVEL_DEBUG = 4,
	MSG_LEVEL_NETWORK_HIGH = 8,
	MSG_LEVEL_CARD_ENGINE = 16,
	MSG_LEVEL_GRAPHICS = 32,
	MSG_LEVEL_NETWORK_LOW = 64,
	MSG_LEVEL_STARTUP = 128,
	MSG_LEVEL_NETWORK_MID = 256,
	MSG_LEVEL_STARTUP_LOW = 512,
	MSG_LEVEL_ALL = UINT32_MAX
};


class StatusLogger
{
public:
	static const uint32_t maxMsgCount = 10;
	void addMessage(uint32_t, std::string);
	uint32_t displayLevel = MSG_LEVEL_URGENT | MSG_LEVEL_USER | MSG_LEVEL_ALL;
	bool printMessages = true;
	//bool displayMessages = false;
	void writeOutMessages();
	uint32_t currentMsgCount = 0;
	std::array<UIText, maxMsgCount> texts;
private:
	std::vector<StatusMessage> messages;
	
	std::string fName = "status.log";



	std::array<std::string, maxMsgCount> strings;


	void evaluateMessages();
};

