#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>

#include "cardEngine.h"

#pragma comment(lib, "Ws2_32.lib")


#define DEFAULT_BUFLEN 512
#define INITIAL_PKTLEN 16
#define STOCK_PKTLEN 52
#define CMD_PKTLEN 32
#define CMD_LENGTH 31

enum serverConfigBits {
	SVRCNF_HC_BITS = 3,
	SVRCNF_HOST_BIT = 1,
	SVRCNF_CLIENT_BIT = 2,
};


struct sockaddr_in2
{
	short   sin_family;
	u_short sin_port;
	struct  in_addr sin_addr;
	char    sin_zero[8];
};
struct HeaderData {
	char packetType;
	char pName[31];
};
struct CmdPacket {
	HeaderData header;
	char cmd[32];
};
struct GamePacket {
	HeaderData header;
	playingCard cardData[52];
};
struct InitPacket {
	HeaderData header;
	char isHost;
	char opName[31];
};
enum packetTypes {
	INIT_PACKET = 1,
	GAME_PACKET = 2,
	CMD_PACKET = 3
};

const uint32_t MAX_PKT_SIZE = 84;

class NetworkUtil {
public:
	char packetData[MAX_PKT_SIZE];
	std::atomic_bool packetReady = false;
	std::atomic_bool packetFinished = true;
	std::atomic_bool sendShutdown = false;
	bool shutdownSent = false;
	bool closeSocket = false;

	SOCKET s;

	std::thread workerThread;

	void startWorker(SOCKET);
	void receivePacket();
	void killWorker();
	void sendPacket(char* data);
	int getBlockSize(HeaderData* hPtr);

	int readDataBlock(int, char*);
	
};

class NetworkingClient {
public:
	NetworkUtil net;

	void initWinsock();
	void negotiateStock();

	void cleanup();

	bool isGameHost = false;
	uint32_t playerIndex = 0;

	std::array<char, 8> usrn;
	std::array<char, 8> oppn;

	std::vector<playingCard>* stockPtr;

	std::string ipaddr;
	int portaddr;


private:
	WSADATA wsaData;
	SOCKET hSocket = INVALID_SOCKET;

	int iResult = 0;

	sockaddr_in2 sockAddr1;


};

struct NetworkServerPair {
	std::array<SOCKET, 2> sockets;
	std::array<bool, 2> connections;

	std::array<playingCard, 52> initialStock;
	std::array<std::array<char, 8>, 2> playerNames;

	uint32_t playerTurn;

	
};

class NetworkingServer
{
public:
	void initWinsock();

private:
	WSADATA wsaData;
	SOCKET lSocket = INVALID_SOCKET;

	const char* sendbuf = "this is a test";
	int iResult;
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;
	sockaddr_in2 remAddr1;
	sockaddr_in2 sockAddr1;

	void readPacket(SOCKET, char*, size_t);

	std::vector<playingCard> stock;
	std::vector<std::array<char, 8>>usrs;

	std::atomic_bool gameRunning = true;
	std::atomic_bool correctPlayerCount = false;
	std::mutex sharedMutex;
	std::atomic_bool stockRecv = false;
	std::atomic_int playerTurn = 1;
	int currentusrs = 0;

	std::vector<SOCKET> cSockets;
	std::vector<std::thread> threads;

	std::array<char, CMD_PKTLEN> command;
	std::atomic_bool commandReady;

	void handleConnection(SOCKET, int);

}; 