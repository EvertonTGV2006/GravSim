#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdlib>
#include <iostream>

#include "networkingClient.h"

#pragma comment(lib, "Ws2_32.lib")


void NetworkingClient::initWinsock() {

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
	sockAddr1.sin_port = htons(80);

	/* inet_addr converts a string with an IP address in dotted format to
	   a long value which is the IP in network byte order.
	   sin_addr.S_un.S_addr specifies the long value in the address union */
	inet_pton(AF_INET, "127.0.0.1", &sockAddr1.sin_addr.S_un.S_addr);


	if (connect(hSocket, (sockaddr*)(&sockAddr1), sizeof(sockAddr1)) != 0)
	{
		throw std::runtime_error("Failed to connect socket");
	}


	iResult = send(hSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(hSocket);
		WSACleanup();
		throw std::runtime_error("Failed to send packet");
	}

	iResult = shutdown(hSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(hSocket);
		WSACleanup();

		throw std::runtime_error("Failed to shutdown socket");
	}

	do {

		iResult = recv(hSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());
		throw std::runtime_error("Failed receive error socket");

	} while (iResult > 0);

	for (uint32_t i = 0; i < DEFAULT_BUFLEN; i++) {
		std::cout << recvbuf[i];
	}


	closesocket(hSocket);
	WSACleanup();


}