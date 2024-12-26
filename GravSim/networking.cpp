#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdlib>
#include <iostream>
#include <fstream>

#include "networking.h"
#include "cardEngine.h"

#pragma comment(lib, "Ws2_32.lib")

void NetworkUtil::receivePacket() {
	int errCode = 0;
	while (sendShutdown == false) {
		if (packetFinished) {
			errCode = readDataBlock(sizeof(HeaderData), &packetData[0]);
			if (errCode == 1) { break; }
			HeaderData* hPtr = reinterpret_cast<HeaderData*>(&packetData[0]);
			int nextBlockSize = getBlockSize(hPtr);

			errCode = readDataBlock(nextBlockSize, &packetData[0] + sizeof(HeaderData));
			if (errCode == 1) { break; }
			packetFinished = false;
			packetReady = true;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}
void NetworkUtil::sendPacket(char* data) {
	HeaderData* hPtr = reinterpret_cast<HeaderData*>(data);
	int packetSize = getBlockSize(hPtr) + sizeof(HeaderData);
	int iResult = send(s, data, packetSize, 0);
	if (iResult != 0) {
		std::cout << WSAGetLastError() << std::endl;
		throw std::runtime_error("Failed to send packet");
	}
}
int NetworkUtil::getBlockSize(HeaderData* hPtr) {
	switch (hPtr->packetType) {
	case INIT_PACKET:
		return sizeof(InitPacket);
		break;
	case GAME_PACKET:
		return sizeof(GamePacket);
		break;
	case CMD_PACKET:
		return sizeof(CmdPacket);
		break;
	}
}
int NetworkUtil::readDataBlock(int blockSize, char* data) {
	char* cursor = data;
	int recvSize = 0;
	int recvTotal = 0;
	int recvCode = 0;
	
	while (recvTotal < blockSize) {
		recvSize = recv(s, cursor, blockSize, 0);
		if (recvSize > 0) {
			recvTotal += recvSize;
		}
		else if (recvSize == 0) {
			return 1; //connection closed
		}
		else {
			//error
			int errCode = WSAGetLastError();
			if (errCode == WSAEINTR) {
				return 1;
			}
			else {
				std::cout << errCode << std::endl;
				throw std::runtime_error("Recv Data Block failed with error");
			}
		}
	}
	return 0;
}
void NetworkUtil::startWorker(SOCKET sock) {
	packetReady = false;
	packetFinished = true;
	sendShutdown = false;
	shutdownSent = false;
	closeSocket = false;
	s = sock;

	workerThread = std::thread(&NetworkUtil::receivePacket, this);
}
void NetworkUtil::killWorker() {
	sendShutdown = true;
	shutdown(s, SD_RECEIVE);
	shutdownSent = true;
	workerThread.join();
}


void NetworkingClient::initWinsock() {
	std::cout << "Initialisng networking..." << std::endl;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		std::cout << iResult << std::endl;
		throw std::runtime_error("Failed to initialse Winsock");
	}

	hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hSocket == INVALID_SOCKET) {
		WSACleanup();
		throw std::runtime_error("Failed to intialise Socket");
	}
	// Set address family
	sockAddr1.sin_family = AF_INET;

	/* Convert port number 80 to network byte order and assign it to
	   the right structure member. */
	sockAddr1.sin_port = htons(portaddr);

	/* inet_addr converts a string with an IP address in dotted format to
	   a long value which is the IP in network byte order.
	   sin_addr.S_un.S_addr specifies the long value in the address union */
	inet_pton(AF_INET, ipaddr.c_str(), &sockAddr1.sin_addr.S_un.S_addr);

	std::cout << "Attempting to connect to " << ipaddr << " on port " << portaddr << std::endl;

	if (connect(hSocket, (sockaddr*)(&sockAddr1), sizeof(sockAddr1)) != 0)
	{
		throw std::runtime_error("Failed to connect socket");
	}
	std::cout << "Connected to Server as ";
	for (size_t i = 0; i < usrn.size(); i++) {
		std::cout << usrn[i];
	}
	std::cout << std::endl;

	//now start worker thread
	net.startWorker(hSocket);

	//now send packet with username

	InitPacket pkt1{};
	pkt1.header.packetType = INIT_PACKET;
	memcpy(&pkt1.header.pName, usrn.data(), usrn.size());

	net.sendPacket(reinterpret_cast<char*>(&pkt1));
	std::cout << "Sent greeting packet, waiting for reply... ";
	//now we wait for a reply...
	while (net.packetReady == false) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::cout << "Recevied reply" << std::endl;
	//now we parse reply;
	InitPacket* pkt2{};
	if (pkt2->header.packetType != INIT_PACKET) {
		std::cout << "Received incorrect packet, exiting" << std::endl;
		throw std::runtime_error("Incorrect Packet");
	}

	isGameHost = (pkt2->isHost == 0) ? true : false;
	net.packetReady = false;
	net.packetFinished = true;

	if (isGameHost) {


		//if game host, we wait for another greeting packet
		std::cout << "Selected as game host, waiting for opponent... ";
		
		
		while (net.packetReady == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		net.packetReady = false;
		net.packetFinished = true;
		if (pkt2->header.packetType != INIT_PACKET) {
			std::cout << "Received incorrect packet, exiting" << std::endl;
			throw std::runtime_error("Incorrect Packet");
		}
		//parse new packet
		std::cout << "Opponent Found: ";
		memcpy(oppn.data(), &(pkt2->opName), oppn.size());
		for (uint32_t i = 0; i < oppn.size(); i++) {
			std::cout << oppn[i];
		}
		std::cout << std::endl;

		//function sends a newgame command
		CmdPacket pkt3{};
		pkt3.header.packetType = CMD_PACKET;
		memcpy(&pkt3.header.pName, usrn.data(), usrn.size());
		std::array<char, 9> cmd = { 0, '/','n','e','w','g','a','m','e' };
		memcpy(&pkt3.cmd, cmd.data(), cmd.size());
		net.sendPacket(reinterpret_cast<char*>(&pkt3));

	}
	else {

		std::cout << "Not hosting game, opponent found: ";
			memcpy(oppn.data(), &(pkt2->opName), oppn.size());
		for (uint32_t i = 0; i < oppn.size(); i++) {
			std::cout << oppn[i];
		}
		std::cout << std::endl;
	}
	


	GamePacket* pkt3{};
	std::cout << "Waiting to receive game details... ";
	while (net.packetReady == false) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	std::cout << "Recveived game details" << std::endl;
	pkt3 = reinterpret_cast<GamePacket*>(&net.packetData);
	net.packetReady = false;
	net.packetFinished = true;
	if (pkt3->header.packetType != GAME_PACKET) {
		std::cout << "Received incorrect packet, exiting" << std::endl;
		throw std::runtime_error("Incorrect Packet");
	}
	stockPtr->resize(52);
	memcpy(stockPtr->data(), pkt3->cardData, stockPtr->size());


}
void NetworkingClient::negotiateStock() {
	if (isGameHost) {
		GamePacket pkt{};
		pkt.header.packetType = GAME_PACKET;
		memcpy(&pkt.header.pName, usrn.data(), usrn.size());
		memcpy(&pkt.cardData, stockPtr->data(), stockPtr->size() * sizeof(playingCard));
		net.sendPacket(reinterpret_cast<char*>(&pkt));
	}
	else {
		GamePacket* pkt3 = reinterpret_cast<GamePacket*>(&net.packetData);
		if (pkt3->header.packetType != GAME_PACKET) {
			std::cout << "Received incorrect packet, exiting" << std::endl;
			throw std::runtime_error("Incorrect Packet");
		}
		stockPtr->resize(52);
		memcpy(stockPtr->data(), pkt3->cardData, stockPtr->size());
		net.packetReady = false;
		net.packetFinished = true;
	}
}

void NetworkingClient::cleanup() {
	net.killWorker();
	shutdown(hSocket, SD_SEND);
	closesocket(hSocket);
	WSACleanup();
}

void NetworkingServer::initWinsock() {
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		std::cout << iResult << std::endl;
		throw std::runtime_error("Failed to initialse Winsock");
	}

	lSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (lSocket == INVALID_SOCKET) {
		WSACleanup();
		throw std::runtime_error("Failed to intialise Socket");
	}




	sockAddr1.sin_family = AF_INET;
	sockAddr1.sin_port = htons(25566);
	sockAddr1.sin_addr.S_un.S_addr = INADDR_ANY; // use default

	// Bind socket to port 80
	if (bind(lSocket, (sockaddr*)(&sockAddr1), sizeof(sockAddr1)) != 0)
	{
		closesocket(lSocket);
		WSACleanup();
		throw std::runtime_error("Failed to bind socket");
	}
	if (listen(lSocket, SOMAXCONN) != 0) {
		closesocket(lSocket);
		WSACleanup();
		throw std::runtime_error("Failed to listen on Socket");
	}

	std::cout << "Listenting" << std::endl;
	while (gameRunning) {
		int iRemoteAddrLen = sizeof(sockaddr_in);
		cSockets.push_back(accept(lSocket, (sockaddr*)&remAddr1, &iRemoteAddrLen));
		std::cout << "Connection being handled" << std::endl;
		if (cSockets.back() == INVALID_SOCKET) {
			closesocket(lSocket);
			WSACleanup();
			throw std::runtime_error("Failed to accept socket");
		}
		//closesocket(lSocket);
		threads.push_back(std::thread(&NetworkingServer::handleConnection, this, cSockets.back(), currentusrs));
	}

	// cleanup
	for (uint32_t i = 0; i < threads.size(); i++) {
		threads[i].join();
	}
	closesocket(lSocket);
	WSACleanup();


}
void NetworkingServer::handleConnection(SOCKET s, int index) {
	std::cout << index << " Handling Connection" << std::endl;
	char initbuf[INITIAL_PKTLEN];
	readPacket(s, &initbuf[0], INITIAL_PKTLEN);
	sharedMutex.lock();
	std::array<char, 8> usrn;
	memcpy(usrn.data(), &initbuf[0], usrn.size() * sizeof(usrn[0]));
	usrs.push_back(usrn);
	currentusrs++;
	sharedMutex.unlock();
	uint32_t serverConfig = 0;
	if (currentusrs == 1) {
		std::cout << "Setting Host" << std::endl;
		serverConfig |= SVRCNF_HOST_BIT;
		memset(&initbuf[0], 0, INITIAL_PKTLEN);
		memcpy(&initbuf[8], &serverConfig, sizeof(serverConfig));
		int sResult = send(s, &initbuf[0], INITIAL_PKTLEN, 0);
		if (sResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Failed to send packet");
		}
		while (correctPlayerCount == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::cout << "Confirming Opponent" << std::endl;
		memcpy(&initbuf[0], &usrs[1], INITIAL_PKTLEN);
		memcpy(&initbuf[8], &serverConfig, sizeof(serverConfig));
		sResult = send(s, &initbuf[0], INITIAL_PKTLEN, 0);
		if (sResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Failed to send packet");
		}
		char stockBuf[STOCK_PKTLEN];
		readPacket(s, &stockBuf[0], STOCK_PKTLEN);
		stock.resize(52);
		memcpy(stock.data(), &stockBuf[0], STOCK_PKTLEN);
		stockRecv = true;
		std::cout << "Recevied Stock" << std::endl;
	}
	else if (currentusrs == 2) {
		std::cout << "Confirming Client" << std::endl;
		correctPlayerCount = true;
		serverConfig |= SVRCNF_CLIENT_BIT;
		memcpy(&initbuf[0], &usrs[0], INITIAL_PKTLEN);
		memcpy(&initbuf[8], &serverConfig, sizeof(serverConfig));
		int sResult = send(s, &initbuf[0], INITIAL_PKTLEN, 0);
		if (sResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Failed to send packet");
		}
		while (stockRecv == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::cout << "Sending Stock" << std::endl;
		sResult = send(s, (char*)stock.data(), STOCK_PKTLEN, 0);
		if (sResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Failed to send packet");
		}
	}
	while (gameRunning) {
		if (index == playerTurn) {
			std::cout << "Waiting for Command" << std::endl;
			readPacket(s, command.data(), CMD_PKTLEN);
			commandReady = true;
			std::string str(command.begin() + 1, command.end());
			if (str == "/disconnect") {
				gameRunning = false;
			}
			while (commandReady == true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		else {
			while (commandReady == false) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			int sResult = send(s, command.data(), CMD_PKTLEN, 0);
			std::cout << "Sent Command" << std::endl;
			if (sResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(s);
				WSACleanup();
				throw std::runtime_error("Failed to send packet");
			}
			playerTurn = (playerTurn + 1) % 2;
			commandReady = false;
		}
	}
	sharedMutex.lock();
	currentusrs--;
	sharedMutex.unlock();
	closesocket(s);
}



void NetworkingServer::readPacket(SOCKET s, char* buf, size_t len) {
	int receivedDataLen = 0;
	size_t cursor = 0;
	int rResult = 0;
	char recvbuf[DEFAULT_BUFLEN];

	do
	{
		rResult = recv(s, recvbuf, DEFAULT_BUFLEN, 0);
		if (rResult > 0) {
			//take data out of recvbuf
			memcpy(buf + cursor, recvbuf, rResult);
			cursor += rResult;
			receivedDataLen += rResult;
		}
		else if (rResult == 0) {
			std::cout << "Connection Closed" << std::endl;
			return;
		}
		else {
			
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Failed receive error socket");
		}
	} while (receivedDataLen < len);
}