#pragma once
#include <cstdlib>
#include <cstdint>
#include <array>
#include <vector>
#include <iostream>

#include "statusLogger.h"

enum cardSuit {
	CARD_SUIT_DIAMONDS = 2,
	CARD_SUIT_CLUBS = 1,
	CARD_SUIT_SPADES = 3,
	CARD_SUIT_HEARTS = 0
};
enum cardRank {
	CARD_RANK_ACE = 1,
	CARD_RANK_2 = 2,
	CARD_RANK_3 = 3,
	CARD_RANK_4 = 4,
	CARD_RANK_5 = 5,
	CARD_RANK_6 = 6,
	CARD_RANK_7 = 7,
	CARD_RANK_8 = 8,
	CARD_RANK_9 = 9,
	CARD_RANK_10= 10,
	CARD_RANK_JACK = 11,
	CARD_RANK_QUEEN = 12,
	CARD_RANK_KING = 13
};
enum handChar {
	HAND_CHAR_Q = 0,
	HAND_CHAR_W = 1,
	HAND_CHAR_E = 2,
	HAND_CHAR_R = 3
};
enum handIndex {
	HAND_INDEX_0 = 'q',
	HAND_INDEX_1 = 'w',
	HAND_INDEX_2 = 'e',
	HAND_INDEX_3 = 'r'
};
enum commandError {
	COMMAND_SUCCESS = 0,
	COMMAND_ERROR_BAD_HAND_INDEX = 1,
	COMMAND_ERROR_BAD_HELD_CARD_INDEX = 2,
	COMMAND_ERROR_BUILD_VALUE_NOT_FOUND = 3,
	COMMAND_ERROR_CARD_TAKE_VALUE_NOT_FOUND = 4,
	COMMAND_ERROR_SECOND_ACTION_NOT_FOUND = 5,
	COMMAND_ERROR_FIRST_ACTION_NOT_FOUND = 6,
	COMMAND_ERROR_BAD_TABLE_INDEX = 7,
	COMMAND_ERROR_BAD_COMMAND_STRING = 8,
	COMMAND_DISCONNECT = 9,
	COMMAND_NEWGAME_SWAP = 10,
	COMMAND_NEWGAME_STICK = 11,
	COMMAND_NOACTION = 12
};
enum commmandAction {
	COMMAND_ACTION_ADD_TO_TABLE = 'a',
	COMMAND_ACTION_STACK_ON_TABLE = 's',
	COMMAND_ACTION_TAKE_FROM_TABLE = 'd',
};
enum commandIDBits {
	COMMAND_BIT_HAND_INDEX = 0,
	COMMAND_BIT_HELD_CARD = 1,
	COMMAND_BIT_COMMAND_ACTION = 2,
	COMMAND_BIT_TABLE_INDEX_START = 3
};

struct playingCard {
	uint8_t data;
	uint8_t suit() {
		return 3 & (data >> 4);
	}
	uint8_t rank() {
		return data & 15; // aces are low
	}
	void setSuit(uint8_t suit) {
		data = (data & 15) | ((suit & 3 )<< 4);
	}
	void setRank(uint8_t rank) {
		data = (data & (~15)) | (rank & 15);
	}
	void print() {

		switch (data & 15)
		{
		case CARD_RANK_ACE:
			std::cout << "Ace of ";
			break;
		case CARD_RANK_2:
			std::cout << "Two of ";
			break;
		case CARD_RANK_3:
			std::cout << "Three of ";
			break;
		case CARD_RANK_4:
			std::cout << "Four of ";
			break;
		case CARD_RANK_5:
			std::cout << "Five of ";
			break;
		case CARD_RANK_6:
			std::cout << "Six of ";
			break;
		case CARD_RANK_7:
			std::cout << "Seven of ";
			break;
		case CARD_RANK_8:
			std::cout << "Eight of ";
			break;
		case CARD_RANK_9:
			std::cout << "Nine of ";
			break;
		case CARD_RANK_10:
			std::cout << "Ten of ";
			break;
		case CARD_RANK_JACK:
			std::cout << "Jack of ";
			break;
		case CARD_RANK_QUEEN:
			std::cout << "Queen of ";
			break;
		case CARD_RANK_KING:
			std::cout << "King of ";
			break;
		default:
			throw std::runtime_error("Invalid card rank");
		}
		switch ((data >> 4) & 3)
		{
		case CARD_SUIT_DIAMONDS:
			std::cout << "Diamonds\t";
			break;
		case CARD_SUIT_CLUBS: std::cout << "Clubs\t"; break;
		case CARD_SUIT_SPADES: std::cout << "Spades\t"; break;
		case CARD_SUIT_HEARTS: std::cout << "Hearts\t"; break;
		default:
			throw std::runtime_error("Invalid card rank");
		}
	}
	uint8_t value() {

		return (rank() == 1) ? suit() * 13 + 12 : suit() * 13 + rank() - 2; //aces are high on the texture
	}
};


struct GameTablePtr {
	std::vector<std::vector<playingCard>*>* hands;
	std::vector<std::vector<playingCard>*>* wins;
	std::vector<std::vector<playingCard>>* table;
	std::vector<playingCard>* stock;
};

struct GameTableData {
	std::vector<std::vector<playingCard>>hands;
	std::vector < std::vector<playingCard>> wins;
	std::vector<std::vector<playingCard>> table;
	std::vector<playingCard> stock;
};

class CardEngine {
public:
	bool gameFinished = false;
	uint32_t handToPlay = 1;
	StatusLogger* stat;

	std::vector<playingCard> stock;
	std::vector<playingCard> DHand;
	std::vector<playingCard> DWin;
	std::vector<playingCard> NDHand;
	std::vector<playingCard> NDWin;
	uint32_t DSweeps;
	uint32_t NDSweeps;
	uint32_t DScore;
	uint32_t NDScore;
	std::vector<std::vector<playingCard>> table;
	uint32_t lastWin = 0;

	playingCard nullStack[8] = { 0,0,0,0,0,0,0,0 };

	std::vector<std::vector<playingCard>*> playerHands;
	std::vector<std::vector<playingCard>*> playerWins;
	std::vector<uint32_t*> playerSweeps;
	std::vector<uint32_t*> playerScores;
	std::vector<std::vector<std::string>*> playerScoreReasons;

	void initStock();
	void setupGame();
	void countScore(uint8_t, uint8_t);
	
	std::vector<std::string> DScoreReasons;
	std::vector<std::string> NDScoreReasons;

	
	GameTablePtr getTable();
	void firstDeal();
	void normalDeal();

	void processTurns();

	void readGameState(std::string);
	void parseNextLine(std::ifstream*, std::vector<playingCard>*);
	void writeGameState(std::string);
	void writeNextLine(std::ofstream*, std::vector<playingCard>*);

	uint32_t cardCommand(std::vector<char>);

	void printCards(uint8_t);

	void populateGameTableData(GameTableData*);
};

struct durakGameState {
	std::vector<playingCard> stock;
	std::array<std::vector<playingCard>, 2> hands;
	std::vector<playingCard> discard;
	std::vector<playingCard> table;
	char trumpSuit;
	void clear();
	void print();
};
class DurakEngine {
public:
	durakGameState state;
	char playerTurn;
	char attacker;
	char winnerID = CHAR_MAX;

	void shuffle();
	void dealGame();
	bool cardCommand(std::string);

};
struct CardData {
	playingCard card;
	glm::vec3 pos;
	float xy;
	float yz;
	float xz;
};
struct CardBuf {
	glm::mat4 mat;
	uint32_t card;
	uint32_t mod1;
	uint32_t mod2;
	uint32_t mod3;
};