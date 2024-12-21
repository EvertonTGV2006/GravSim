#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


#define DEFAULT_BUFLEN 512

struct sockaddr_in2
{
	short   sin_family;
	u_short sin_port;
	struct  in_addr sin_addr;
	char    sin_zero[8];
};

class NetworkingClient {
private:
	WSADATA wsaData;
	SOCKET hSocket = INVALID_SOCKET;

	const char* sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	sockaddr_in2 sockAddr1;

	void initWinsock();
	


};