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

const uint32_t MAX_PKT_SIZE = 256;

class NetworkUtil {
public:
	char packetData[MAX_PKT_SIZE]; //weird pointer getting overwritten becasue packetdata was too small
	std::atomic_bool packetReady = false;
	std::atomic_bool packetFinished = true;
	std::atomic_bool sendShutdown = false;
	std::atomic_bool connectionClosed = false;
	bool shutdownSent = false;
	bool closeSocket = false;

	void startWorker(SOCKET*);
	void killWorker();
	void sendPacket(char* data);

private:
	SOCKET* sock;

	std::thread workerThread;

	
	void receivePacket();
	
	
	int getBlockSize(HeaderData* hPtr);

	int readDataBlock(int, char*);
	
};

struct NetworkServerPair {
	std::array<NetworkUtil*, 2> net;
	playingCard initialStock[52];
	std::array<std::array<char, 8>, 2> playerNames;
	uint32_t playerTurn;
	bool hostDealer = false;
	bool partComplete = false;
	bool complete = false;
	bool exitRequired = false;
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

class NetworkingServer
{
public:
	void initWinsock();

private:
	WSADATA wsaData;
	SOCKET lSocket = INVALID_SOCKET;
	

	SOCKET socks[128];
	NetworkUtil nets[128];
	uint32_t currentFreeNet = 0;

	int iResult;
	sockaddr_in2 remAddr1;
	int iRemoteAddrLen = sizeof(sockaddr);
	sockaddr_in2 sockAddr1;

	std::array<char, 8> newgameChar = { '/','n','e','w','g','a','m','e' };

	NetworkServerPair nullPair{};
	NetworkServerPair unfinishedPair{};

	std::atomic_bool exitTrigger = false;
	std::atomic_bool qSocketReady = false;
	std::atomic_bool qSocketHandled = true;

	std::vector<NetworkServerPair> gamePairs;

	void workerListen();
	void handleConnections();
	void handleNewgame(NetworkServerPair*);
}; 