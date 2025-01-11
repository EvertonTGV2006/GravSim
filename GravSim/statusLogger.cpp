#include "statusLogger.h"
#include <fstream>
#include <chrono>
#include <format>

void StatusLogger::addMessage(uint32_t level, std::string msg) {
	StatusMessage newMessage{};
	newMessage.level = level;
	std::chrono::time_point msgTime = std::chrono::system_clock::now();
	std::string timeStr = std::format("{:%H:%M:%S}", msgTime);
	timeStr.resize(11);
	newMessage.message = "[" + timeStr + "] " + msg;
	messages.push_back(newMessage);
	evaluateMessages();
	if (printMessages) { std::cout << newMessage.message << "\n"; }
}

void StatusLogger::evaluateMessages() {


	UIText baseText{};
	baseText.config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_TRUE | UI_DATA_STRING;
	baseText.colour = glm::vec3(1);

	
	//backwards from vector
	uint32_t msgsFound = 0;
	for (int i = static_cast<int>(messages.size() - 1); i > 0; i--) {
		if (messages[i].level & displayLevel) {
			//if message level bit is flagged in display level
			texts[msgsFound] = baseText;
			texts[msgsFound].dataP = &strings[msgsFound];
			strings[msgsFound] = messages[i].message;
			if (msgsFound == 0) { texts[msgsFound].config = UI_ALIGNMENT_H_L | UI_ALIGNMENT_V_T | UI_NEWLINE_FALSE | UI_DATA_STRING; }
			msgsFound++; 
			if (msgsFound == maxMsgCount) {
				break;
			}
			
		}
	}
	currentMsgCount = msgsFound;
}
void StatusLogger::writeOutMessages() {
	std::ofstream file(fName);
	addMessage(MSG_LEVEL_URGENT, "Writing out log");
	for (uint32_t i = 0; i < messages.size(); i++) {
		file << messages[i].level << "\t" << messages[i].message << "\n";
	}
	file.close();

}