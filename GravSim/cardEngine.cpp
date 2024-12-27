#include "cardEngine.h"

#include <cstdlib>
#include <cstdint>
#include <array>
#include <vector>
#include <fstream>
#include <string>

#include <random>
#include <algorithm>


void CardEngine::initStock() {
	stock.clear();
	uint8_t cardIndex = 0;
	for (uint8_t suit = 0; suit < 4; suit++) {
		for (uint8_t rank = 1; rank < 14; rank++) {
			playingCard card{};
			card.setSuit(suit);
			card.setRank(rank);
			stock.push_back(card);
		}
	}
	std::random_device rd;
	std::mt19937 gen{ rd() };
	//printCards(1);
	std::ranges::shuffle(stock, gen);

}


void CardEngine::setupGame() {

	playerHands.clear();
	playerWins.clear();
	playerScoreReasons.clear();
	playerScores.clear();
	playerSweeps.clear();

	table.clear();
	NDHand.clear();
	DHand.clear();
	NDWin.clear();
	DWin.clear();


	NDScore = 0;
	DScore = 0;
	lastWin = 0;

	DScoreReasons.clear();
	NDScoreReasons.clear();







	firstDeal();
	//printCards(0);

	playerHands.push_back(&NDHand);
	playerHands.push_back(&DHand);
	playerWins.push_back(&NDWin);
	playerWins.push_back(&DWin);
	playerSweeps.push_back(&NDSweeps);
	playerSweeps.push_back(&DSweeps);
	playerScores.push_back(&NDScore);
	playerScores.push_back(&DScore);
	playerScoreReasons.push_back(&NDScoreReasons);
	playerScoreReasons.push_back(&DScoreReasons);
}

void CardEngine::firstDeal() {
	std::vector<playingCard> newStack = {};
	for (uint8_t i = 0; i < 2; i++) {
		NDHand.push_back(stock.back());
		stock.pop_back();
		NDHand.push_back(stock.back());
		stock.pop_back();
		newStack = { stock.back() };
		table.push_back(newStack);
		stock.pop_back();
		newStack = { stock.back() };
		table.push_back(newStack);
		stock.pop_back();
		DHand.push_back(stock.back());
		stock.pop_back();
		DHand.push_back(stock.back());
		stock.pop_back();

	}
}

void CardEngine::normalDeal() {
	for (uint8_t i = 0; i < 2; i++) {
		NDHand.push_back(stock.back());
		stock.pop_back();
		NDHand.push_back(stock.back());
		stock.pop_back();
		DHand.push_back(stock.back());
		stock.pop_back();
		DHand.push_back(stock.back());
		stock.pop_back();
	}
}

void CardEngine::printCards(uint8_t opt) {
	if (opt == 1) {
		std::cout << "Stock: " << stock.size() << std::endl;
		for (uint8_t i = 0; i < stock.size(); i++) {
			stock[i].print();
			if (i % 8 == 7) {
				std::cout << std::endl;
			}
		}
		std::cout << std::endl;
	}

	std::cout << "Dealer: " << DHand.size() << std::endl;
	for (uint8_t i = 0; i < DHand.size(); i++) {
		DHand[i].print();
		if (i % 8 == 7) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;

	std::cout << "Non-Dealer: " << NDHand.size() << std::endl;
	for (uint8_t i = 0; i < NDHand.size(); i++) {
		NDHand[i].print();
		if (i % 8 == 7) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;

	std::cout << "Table: " << NDHand.size() << std::endl;
	for (uint8_t i = 0; i < table.size(); i++) {
		for(uint8_t j = 0; j < table[i].size(); j++)
		table[i][j].print();
		if (i % 8 == 7) {
			std::cout << std::endl;
		}
	}
}

uint32_t CardEngine::cardCommand(std::vector<char> command) {
	printCards(0);

	for (uint32_t i = 0; i < command.size(); i++) {
		std::cout << command[i];
	}
	std::cout << std::endl;

	if (command.size() < 3) {
		return COMMAND_ERROR_BAD_HAND_INDEX;
	}

	char handIndex = command[COMMAND_BIT_HAND_INDEX];
	if (handIndex != 0 && handIndex != 1) { return COMMAND_ERROR_BAD_HAND_INDEX; }
	std::vector<playingCard>* currentHand = playerHands[handIndex];
	std::vector<playingCard>* currentWin = playerWins[handIndex];

	char heldCardIndexRaw = command[COMMAND_BIT_HELD_CARD];
	uint8_t heldCardIndex;
	if (heldCardIndexRaw == HAND_INDEX_0) { heldCardIndex = 0; }
	else if (heldCardIndexRaw == HAND_INDEX_1) { heldCardIndex = 1; }
	else if (heldCardIndexRaw == HAND_INDEX_2) { heldCardIndex = 2; }
	else if (heldCardIndexRaw == HAND_INDEX_3) { heldCardIndex = 3; }
	else if (heldCardIndexRaw == '/') {
		std::string commandString(command.begin() + 2, command.end());
		//if (commandString == "newgame") {
		//	setupGame();
		//	return COMMAND_SUCCESS;
		//}
		if (commandString == "save") {
			writeGameState("game.txt");
			return COMMAND_NOACTION;
		}
		if (commandString == "load") {
			readGameState("game.txt");
			return COMMAND_SUCCESS;
		}
		return COMMAND_ERROR_BAD_COMMAND_STRING;

	}
	else { return COMMAND_ERROR_BAD_HELD_CARD_INDEX; }

	if (heldCardIndex >= currentHand->size()) {
		return COMMAND_ERROR_BAD_HAND_INDEX;
	}
	playingCard heldCard = (*currentHand)[heldCardIndex];

	char commandAction = command[COMMAND_BIT_COMMAND_ACTION];

	if (commandAction == COMMAND_ACTION_ADD_TO_TABLE) {
		std::vector<playingCard> newStack{};
		newStack.push_back(heldCard);
		table.push_back(newStack);
		currentHand->erase(currentHand->begin() + heldCardIndex);
	}
	else if (commandAction == COMMAND_ACTION_STACK_ON_TABLE) {
		char targetStackIndexRaw = command[COMMAND_BIT_TABLE_INDEX_START];
		uint8_t targetStackIndex = targetStackIndexRaw - 49;

		if (targetStackIndex >= table.size()) {
			return COMMAND_ERROR_BAD_TABLE_INDEX;
		}
		std::vector<playingCard> targetStack = table[targetStackIndex];

		uint8_t stackValue = 0;
		for (uint8_t i = 0; i < targetStack.size(); i++) {
			stackValue += targetStack[i].rank();
		}

		uint8_t resultValue = stackValue + heldCard.rank();

		bool stackValueFoundFlag = false;
		for (uint8_t i = 0; i < currentHand->size(); i++) {
			if (stackValueFoundFlag == false && resultValue == (*currentHand)[i].rank() && i != heldCardIndex) { stackValueFoundFlag = true; }
		}
		if (stackValueFoundFlag == false) {
			return COMMAND_ERROR_BUILD_VALUE_NOT_FOUND;
		}

		table[targetStackIndex].push_back(heldCard);
		currentHand->erase(currentHand->begin() + heldCardIndex);
	}
	else if (commandAction == COMMAND_ACTION_TAKE_FROM_TABLE) {
		uint8_t addedStacksValue = 0;
		char targetStackIndexRaw = 0;
		uint8_t targetStackIndex = 0;
	
		//1st step check every combo is legitimate
		for (uint8_t i = COMMAND_BIT_TABLE_INDEX_START; i < command.size(); i++) {
			if ((i - COMMAND_BIT_TABLE_INDEX_START) % 2 == 0) {
				targetStackIndexRaw = command[i];
				targetStackIndex = targetStackIndexRaw - 49;
				if (targetStackIndex >= table.size()) {
					return COMMAND_ERROR_BAD_TABLE_INDEX;
				}
				std::vector<playingCard> targetStack = table[targetStackIndex];
				for (uint8_t i = 0; i < targetStack.size(); i++) {
					addedStacksValue += targetStack[i].rank();
				}
			}
			else if ((i - COMMAND_BIT_TABLE_INDEX_START) % 2 == 1) {
				if (command[i] == ',') {
					if (addedStacksValue != heldCard.rank()) {
						return COMMAND_ERROR_CARD_TAKE_VALUE_NOT_FOUND;
					}
					addedStacksValue = 0;
				}
				else if (command[i] == '+') {
					if (heldCard.rank() == CARD_RANK_KING || heldCard.rank() == CARD_RANK_QUEEN || heldCard.rank() == CARD_RANK_JACK) {
						return COMMAND_ERROR_CARD_TAKE_VALUE_NOT_FOUND;
					}
				}
				else {
					return COMMAND_ERROR_SECOND_ACTION_NOT_FOUND;
				}
			}
		}
		if (addedStacksValue != heldCard.rank()) {
			return COMMAND_ERROR_CARD_TAKE_VALUE_NOT_FOUND;
		}

		//if passed then move all the cards
		for (uint8_t i = COMMAND_BIT_TABLE_INDEX_START; i < command.size(); i++) {
			if ((i - COMMAND_BIT_TABLE_INDEX_START) % 2 == 0) {
				targetStackIndexRaw = command[i];
				targetStackIndex = targetStackIndexRaw - 49;
				while (table[targetStackIndex].size() > 0) {
					currentWin->push_back(table[targetStackIndex].back());
					table[targetStackIndex].pop_back();
				}
				
			}
		}
		for (uint8_t i = 0; i < table.size(); i++) {
			if (table[i].size() == 0) {
				table.erase(table.begin() + i);
				i--;
			}
		}
		currentWin->push_back(heldCard);
		lastWin = handIndex;
		currentHand->erase(currentHand->begin() + heldCardIndex);
		if (table.size() == 0) {
			(*(playerSweeps[handIndex]))++;
		}


	}
	else {
		
		return COMMAND_ERROR_FIRST_ACTION_NOT_FOUND;
	}
	if (DHand.size() == 0) {
		if (stock.size() == 0) {
			for (uint8_t i = 0; i < table.size(); i++) {
				while (table[i].size() > 0) {
					playerWins[handIndex]->push_back(table[i].back());
					table[i].pop_back();
				}
			}
			countScore(0, 0);
			countScore(1, 1);
			gameFinished = true;
		}
		else {
			normalDeal();
		}
	}


	//printCards(0);
	return COMMAND_SUCCESS;


}

void CardEngine::processTurns() {
	uint32_t player = 0;
	while (
		table.size() > 0 || 
		/*/stock.size() > 0 ||*/
		DHand.size() > 0 || 
		NDHand.size() > 0) {

		if (DHand.size() == 0 && stock.size() >= 4) {
			normalDeal();
		}
		std::string inputCommand;
		printCards(0);
		std::cout << std::endl << "Player " << player << " Command: ";
		std::cin >> inputCommand;

		

		std::vector<char> commandVector;
		commandVector.push_back(player);
		for (uint8_t i = 0; i < inputCommand.size(); i++) {
			commandVector.push_back(inputCommand[i]);
		}
		if (commandVector[1] == '!') {
			std::cout << "BREAK" << std::endl;
		}
		uint32_t retCode = cardCommand(commandVector);
		std::cout << retCode<<std::endl;
		if (retCode == 0) {
			player = (player + 1 ) % 2;
		}
		
	}
	countScore(0, 0);
	countScore(1, 0);
	std::cout << "NDScore: " << uint32_t(NDScore) << " DScore: " << uint32_t(DScore) << std::endl;
	for (uint32_t i = 0; i < NDScoreReasons.size(); i++) {
		std::cout << NDScoreReasons[i] << std::endl;
	}
}

void CardEngine::countScore(uint8_t handIndex, uint8_t scoreIndex){
	
	std::vector<std::string>* localScoreReasons = playerScoreReasons[handIndex];
	playingCard scoreCard;
	uint8_t localScore = 0;
	uint8_t spadeCount = 0;
	for (uint8_t i = 0; i < playerWins[handIndex]->size(); i++) {
		scoreCard = (*playerWins[handIndex])[i];
		if (scoreCard.rank() == CARD_RANK_ACE) {
			localScore++;
			if (scoreCard.suit() == CARD_SUIT_DIAMONDS) { localScoreReasons->push_back("Ace of Diamonds"); }
			else if (scoreCard.suit() == CARD_SUIT_CLUBS) { localScoreReasons->push_back("Ace of Clubs"); }
			else if (scoreCard.suit() == CARD_SUIT_SPADES) { localScoreReasons->push_back("Ace of Spades"); }
			else if (scoreCard.suit() == CARD_SUIT_HEARTS) { localScoreReasons->push_back("Ace of Hearts"); }
		}
		if (scoreCard.suit() == CARD_SUIT_SPADES) {
			spadeCount++;
		}
		if (scoreCard.suit() == CARD_SUIT_SPADES && scoreCard.rank() == CARD_RANK_2) {
			localScore++;
			localScoreReasons->push_back("Little Casino");
		}
		if (scoreCard.suit() == CARD_SUIT_DIAMONDS && scoreCard.rank() == CARD_RANK_10) {
			localScore += 2;
			localScoreReasons->push_back("Big Casino");
		}
	}
	if (spadeCount >= 7) {
		localScore++;
		localScoreReasons->push_back("Most Spades");
	}
	if (playerWins[handIndex]->size() > 26) {
		localScore += 3;
		localScoreReasons->push_back("Most Cards");
	}

	for (uint32_t i = 0; i < *playerSweeps[handIndex]; i++) {
		localScoreReasons->push_back("Sweep");
	}

	*playerScores[handIndex] = localScore + *playerSweeps[handIndex];

	std::cout << handIndex << ": " << uint32_t(*playerScores[handIndex]) << std::endl;
}

GameTablePtr CardEngine::getTable() {
	GameTablePtr gt{};
	gt.table = &table;
	gt.stock = &stock;
	gt.hands = &playerHands;
	gt.wins = &playerWins;

	return gt;
}

void CardEngine::populateGameTableData(GameTableData* gt) {
	gt->hands.clear();
	gt->wins.clear();
	gt->table.clear();
	gt->stock.clear();
	gt->hands.push_back(NDHand);
	gt->hands.push_back(DHand);
	gt->wins.push_back(NDWin);
	gt->wins.push_back(DWin);
	gt->table = table;
	gt->stock = stock;
}

void CardEngine::readGameState(std::string path) {
	std::ifstream file(path);
	if (file.is_open() == false) {
		return;
	}
	parseNextLine(&file, &NDHand);
	parseNextLine(&file, &DHand);
	parseNextLine(&file, &NDWin);
	parseNextLine(&file, &DWin);
	parseNextLine(&file, &stock);
	std::vector<playingCard> tableStack{};
	table.clear();
	while (file.peek() != EOF) {
		parseNextLine(&file, &tableStack);
		table.push_back(tableStack);
	}

	file.close();
}
void CardEngine::parseNextLine(std::ifstream* file, std::vector<playingCard>* data) {
	std::string value;
	playingCard card;
	char readChar;
	data->clear();
	while (true) {
		readChar = file->get();
		if (readChar == ',') {
			card.data = std::stoi(value);
			data->push_back(card);
			value.clear();
		}
		else if (readChar == '\n') {
			if (value.empty() == true) {
				return;
			}
			card.data = std::stoi(value);
			data->push_back(card);
			value.clear();
		}
		else {
			value.push_back(readChar);
		}
	}
}
void CardEngine::writeGameState(std::string path) {
	std::ofstream file(path);
	if (file.is_open() == false) {
		throw std::runtime_error("Failed to write gameState");
	}
	writeNextLine(&file, &NDHand);
	writeNextLine(&file, &DHand);
	writeNextLine(&file, &NDWin);
	writeNextLine(&file, &DWin);
	writeNextLine(&file, &stock);
	for (uint8_t i = 0; i < table.size(); i++) {
		writeNextLine(&file, &table[i]);
	}
	file.close();
}
void CardEngine::writeNextLine(std::ofstream* file, std::vector<playingCard>* data) {
	for (uint8_t i = 0; i < data->size(); i++) {
		*file << static_cast<int>((*data)[i].data) << ',';
	}
	*file << '\n';
}