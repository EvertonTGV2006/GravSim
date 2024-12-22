#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdlib>
#include <iostream>
#include <fstream>

#include "networking.h"
#include "cardEngine.h"

#pragma comment(lib, "Ws2_32.lib")


void NetworkingClient::initWinsock() {
	fout.open("netout.txt");

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
	sockAddr1.sin_port = htons(25566);

	/* inet_addr converts a string with an IP address in dotted format to
	   a long value which is the IP in network byte order.
	   sin_addr.S_un.S_addr specifies the long value in the address union */
	inet_pton(AF_INET, "77.100.95.113", &sockAddr1.sin_addr.S_un.S_addr);


	if (connect(hSocket, (sockaddr*)(&sockAddr1), sizeof(sockAddr1)) != 0)
	{
		throw std::runtime_error("Failed to connect socket");
	}
	std::cout << "Connected to Server as ";
	for (size_t i = 0; i < usrn.size(); i++) {
		std::cout << usrn[i];
	}
	std::cout << std::endl;




	//send inital packet

	size_t cursor = 0;
	size_t cpySize = usrn.size() * sizeof(usrn[0]);
	memcpy(&initialPacket + cursor, usrn.data(), cpySize);
	cursor += cpySize;
	cpySize = sizeof(versionMajor);
	memcpy(&initialPacket + cursor, &versionMajor, cpySize);
	cursor += cpySize;
	cpySize = sizeof(versionMinor);
	memcpy(&initialPacket + cursor, &versionMinor, cpySize);

	iResult = send(hSocket, &(initialPacket)[0], INITIAL_PKTLEN, 0);
	//iResult = send(hSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(hSocket);
		WSACleanup();
		throw std::runtime_error("Failed to send packet");
	}

	//now wait for return packet with pName param and host/client config + 4 bytes padding
	char replyPacket[INITIAL_PKTLEN];
	readPacket(hSocket, &replyPacket[0], INITIAL_PKTLEN);
	cursor = 0;
	cpySize = sizeof(oppn[0]) * oppn.size();
	memcpy(oppn.data(), &replyPacket[0] + cursor, cpySize);
	cursor += cpySize;
	cpySize = sizeof(serverConfig);
	memcpy(&serverConfig, &replyPacket[0] + cursor, cpySize);

	if ((serverConfig & SVRCNF_HC_BITS) == SVRCNF_HOST_BIT) {

		std::cout << "Waiting for opponent... " << std::endl;
		readPacket(hSocket, &replyPacket[0], INITIAL_PKTLEN);
		cursor = 0;
		cpySize = sizeof(oppn[0]) * oppn.size();
		memcpy(oppn.data(), &replyPacket[0] + cursor, cpySize);
	}
	std::cout << "Opponent Found: ";
	for (size_t i = 0; i < oppn.size(); i++) {
		std::cout << oppn[i];
	}
	std::cout << std::endl;






	//iResult = shutdown(hSocket, SD_SEND);
	//if (iResult == SOCKET_ERROR) {
	//	printf("shutdown failed with error: %d\n", WSAGetLastError());
	//	closesocket(hSocket);
	//	WSACleanup();

	//	throw std::runtime_error("Failed to shutdown socket");
	//}

	//do {

	//	iResult = recv(hSocket, recvbuf, recvbuflen, 0);
	//	if (iResult > 0) {
	//		printf("Bytes received: %d\n", iResult);
	//	}
	//	else if (iResult == 0) {
	//		printf("Connection closed\n");
	//	}
	//	else {
	//		printf("recv failed with error: %d\n", WSAGetLastError());
	//		std::cout << iResult << std::endl;
	//		std::cout << WSAGetLastError() << std::endl;
	//		throw std::runtime_error(("Failed receive error socket"));
	//	}

	//} while (iResult > 0);

	//for (uint32_t i = 0; i < DEFAULT_BUFLEN; i++) {
	//	std::cout << recvbuf[i];
	//}


	//closesocket(hSocket);
	//WSACleanup();


}
void NetworkingClient::sendStock(std::vector<playingCard>* stock) {
	int sResult = send(hSocket, (char*)stock->data(), STOCK_PKTLEN, 0);
	if (sResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(hSocket);
		WSACleanup();
		throw std::runtime_error("Failed to send packet");
	}
}
void NetworkingClient::recvStock(std::vector<playingCard>* stock) {
	char buf[STOCK_PKTLEN];
	readPacket(hSocket, &buf[0], STOCK_PKTLEN);
	stock->clear();
	stock->resize(STOCK_PKTLEN);
	memcpy(stock->data(), &buf[0], STOCK_PKTLEN);
}
void NetworkingClient::sendCmd(std::vector<char>* cmd) {
	char buf[CMD_PKTLEN]{};
	memcpy(&buf[0], cmd->data(), cmd->size());
	int sResult = send(hSocket, &buf[0], CMD_PKTLEN, 0);
	if (sResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(hSocket);
		WSACleanup();
		throw std::runtime_error("Failed to send packet");
	}
}
void NetworkingClient::recvCmd(std::vector<char>* cmd) {
	cmd->clear();
	char buf[CMD_PKTLEN];
	readPacket(hSocket, &buf[0], CMD_PKTLEN);
	size_t endIndex = 0;
	for (int i = CMD_PKTLEN - 1; i >= 0; i--) {
		if (buf[i] != 0) {
			endIndex = i;
			break;
		}
	}
	for (int i = 0; i <= endIndex; i++) {
		cmd->push_back(buf[i]);
	}
}

void NetworkingClient::readPacket(SOCKET s, char* buf, size_t len) {
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
		}
		else {
			std::cout<< WSAGetLastError();
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Packet Read Failed");
		}
	} while (receivedDataLen < len);
}
void NetworkingClient::cleanup() {
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
		}
		else {
			
			closesocket(s);
			WSACleanup();
			throw std::runtime_error("Failed receive error socket");
		}
	} while (receivedDataLen < len);
}