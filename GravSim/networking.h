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

class NetworkingClient {
public:
	std::fstream fout;

	void initWinsock();
	void sendStock(std::vector<playingCard>*);
	void recvStock(std::vector<playingCard>*);
	void sendCmd(std::vector<char>*);
	void recvCmd(std::vector<char>*);
	void cleanup();


	std::array<char, 8> usrn;
	uint32_t versionMajor;
	uint32_t versionMinor;

	std::array<char, 8> oppn;
	uint32_t serverConfig;


private:
	WSADATA wsaData;
	SOCKET hSocket = INVALID_SOCKET;


	char initialPacket[INITIAL_PKTLEN];
	const char* sendbuf = "this is a test";
	char recvbuf2[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	sockaddr_in2 sockAddr1;
	
	void readPacket(SOCKET, char*, size_t);

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