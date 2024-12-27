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
			if (errCode == 1) { sendShutdown = true; break; }
			HeaderData* hPtr = reinterpret_cast<HeaderData*>(&packetData[0]);
			int nextBlockSize = getBlockSize(hPtr);

			errCode = readDataBlock(nextBlockSize, &packetData[0] + sizeof(HeaderData));
			if (errCode == 1) { sendShutdown = true; break; }
			packetFinished = false;
			std::cout << "Received packet of size " << nextBlockSize + sizeof(HeaderData) << std::endl;
			packetReady = true;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}
void NetworkUtil::sendPacket(char* data) {
	if (sendShutdown == false) {
		HeaderData* hPtr = reinterpret_cast<HeaderData*>(data);
		int packetSize = getBlockSize(hPtr) + sizeof(HeaderData);
		//std::cout << "SocketPtr: " << sock << "SocketObj: " << *sock << std::endl;
		int iResult = send(*sock, data, packetSize, 0);
		//std::cout << "SocketPtr: " << sock << "SocketObj: " << *sock << std::endl;
		if (iResult == SOCKET_ERROR) {
			int errCode = WSAGetLastError();
			std::cout << errCode << std::endl;
			if (errCode == WSAEINTR || errCode == WSAECONNRESET) {
				sendShutdown = true;
			}
			else {
				throw std::runtime_error("Failed to send packet");
			}
		}
		std::cout << "Sent packet of size " << packetSize << " with x bytes sent " << iResult << std::endl;
	}
}
int NetworkUtil::getBlockSize(HeaderData* hPtr) {
	switch (hPtr->packetType) {
	case INIT_PACKET:
		return sizeof(InitPacket)-sizeof(HeaderData);
		break;
	case GAME_PACKET:
		return sizeof(GamePacket)-sizeof(HeaderData);
		break;
	case CMD_PACKET:
		return sizeof(CmdPacket) -sizeof(HeaderData); 
		break;
	}
	return 0;
}
int NetworkUtil::readDataBlock(int blockSize, char* data) {
	char* cursor = data;
	int recvSize = 0;
	int recvTotal = 0;
	int recvCode = 0;
	//std::cout << "SocketPtr: " << sock << "SocketObj: " << *sock << std::endl;
	while (recvTotal < blockSize) {
		recvSize = recv(*sock, cursor, blockSize, 0);
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
			else if (errCode == WSAECONNRESET) { return 1; }
			else {
				std::cout << INVALID_SOCKET << std::endl;
				std::cout << *sock << std::endl;
				std::cout << errCode << std::endl;
				throw std::runtime_error("Recv Data Block failed with error");
			}
		}
	}
	return 0;
}
void NetworkUtil::startWorker(SOCKET* sockPtr) {
	packetReady = false;
	packetFinished = true;
	sendShutdown = false;
	shutdownSent = false;
	closeSocket = false;
	sock = sockPtr;

	workerThread = std::thread(&NetworkUtil::receivePacket, this);
}
void NetworkUtil::killWorker() {
	sendShutdown = true;
	shutdown(*sock, SD_BOTH);
	closesocket(*sock);
	shutdownSent = true;
	workerThread.join();
	connectionClosed = true;
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
	net.startWorker(&hSocket);

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
	InitPacket* pkt2 = reinterpret_cast<InitPacket*>(&net.packetData);
	if (pkt2->header.packetType != INIT_PACKET) {
		std::cout << "Received incorrect packet, exiting" << std::endl;
		throw std::runtime_error("Incorrect Packet");
	}

	isGameHost = (pkt2->isHost == 0) ? true : false;
	std::cout << "Is Game Host? : " << isGameHost << "\t";
	net.packetReady = false;
	net.packetFinished = true;

	if (isGameHost) {


		//if game host, we wait for another greeting packet
		std::cout << "Selected as game host, waiting for opponent... ";
		
		
		while (net.packetReady == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

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
		net.packetReady = false;
		net.packetFinished = true;

		//function sends a newgame command
		CmdPacket pkt3{};
		pkt3.header.packetType = CMD_PACKET;
		memcpy(&pkt3.header.pName, usrn.data(), usrn.size());
		std::array<char, 9> cmd = { 0,'/','n','e','w','g','a','m','e' };
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
		net.packetReady = false;
		net.packetFinished = true;
	}
	


	//GamePacket* pkt3{};
	//std::cout << "Waiting to receive game details... ";
	//while (net.packetReady == false) {
	//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	//}
	//std::cout << "Recveived game details" << std::endl;
	//pkt3 = reinterpret_cast<GamePacket*>(&net.packetData);
	//if (pkt3->header.packetType != GAME_PACKET) {
	//	std::cout << "Received incorrect packet, exiting" << std::endl;
	//	throw std::runtime_error("Incorrect Packet");
	//}
	//stockPtr->resize(52);
	//memcpy(stockPtr->data(), pkt3->cardData, stockPtr->size());
	//net.packetReady = false;
	//net.packetFinished = true;

}
void NetworkingClient::negotiateStock() {
	if (isGameHost) {
		GamePacket pkt{};
		pkt.header.packetType = GAME_PACKET;
		memcpy(&pkt.header.pName, usrn.data(), usrn.size());
		memcpy(&pkt.cardData, stockPtr->data(), stockPtr->size() * sizeof(playingCard));
		net.sendPacket(reinterpret_cast<char*>(&pkt));
		std::cout << "Sent stock to server" << std::endl;
	}
	else {
		GamePacket* pkt3 = reinterpret_cast<GamePacket*>(&net.packetData);
		while (net.packetReady == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		if (pkt3->header.packetType != GAME_PACKET) {
			std::cout << "Received incorrect packet, exiting" << std::endl;
			throw std::runtime_error("Incorrect Packet");
		}
		stockPtr->resize(52);
		memcpy(stockPtr->data(), pkt3->cardData, stockPtr->size());
		net.packetReady = false;
		net.packetFinished = true;
		std::cout << "Received stock from server";
	}
	
}
void NetworkingClient::cleanup() {
	std::cout << "Cleaning up networking... " << std::endl;
	net.killWorker();
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

	std::thread workerThread(&NetworkingServer::workerListen, this);

	handleConnections();
}
void NetworkingServer::workerListen() {
	while (exitTrigger == false) {
		while (qSocketHandled == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); //wait for queued socket to be handled by main thread
		}
		socks[currentFreeNet] = INVALID_SOCKET;
		socks[currentFreeNet] = accept(lSocket, (sockaddr*)&remAddr1, &iRemoteAddrLen);
		if (socks[currentFreeNet] == INVALID_SOCKET) {
			std::cout << WSAGetLastError() << std::endl;
			std::cout << "Error accepting, invalid socket, ignoring... " << std::endl;
		}
		else {
			qSocketHandled = false;
			qSocketReady = true;
		}
	}
}
void NetworkingServer::handleConnections() {
	while (exitTrigger == false) {
		//std::cout << "Handling Connections..." << std::endl;
		if (qSocketReady == true) {
			std::cout << "Incoming socket...\t";
			if (unfinishedPair.partComplete == false) {
				unfinishedPair.net[0] = &nets[currentFreeNet];
				unfinishedPair.net[0]->startWorker(&socks[currentFreeNet]);
				unfinishedPair.partComplete = true;
			}
			else {
				unfinishedPair.net[1] = &nets[currentFreeNet];
				unfinishedPair.net[1]->startWorker(&socks[currentFreeNet]);
				unfinishedPair.complete = true;
			}
			currentFreeNet++;
				
			if (unfinishedPair.complete == false) {
				//receive hostname and send back host confirmation;
				InitPacket pkt1{};
				pkt1.header.packetType = INIT_PACKET;
				pkt1.isHost = 0;
				unfinishedPair.net[0]->sendPacket(reinterpret_cast<char*>(&pkt1));
				std::cout << " Assigned as host...\t";
				while (unfinishedPair.net[0]->packetReady == false) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				InitPacket* pkt2 = reinterpret_cast<InitPacket*>(&unfinishedPair.net[0]->packetData);
				if (pkt2->header.packetType != INIT_PACKET) {
					throw std::runtime_error("Incorrect Packet");
				}
				std::cout << "Received Name\t" << std::endl;
				memcpy(unfinishedPair.playerNames[0].data(), &(pkt2->header.pName), unfinishedPair.playerNames[0].size());
				//load hostname into memory;
				unfinishedPair.net[0]->packetReady = false;
				unfinishedPair.net[0]->packetFinished = true;
			}
			else {
				InitPacket pkt1{};
				pkt1.header.packetType = INIT_PACKET;
				pkt1.isHost = 1;
				memcpy(&pkt1.opName, unfinishedPair.playerNames[0].data(), unfinishedPair.playerNames[0].size());
				unfinishedPair.net[1]->sendPacket(reinterpret_cast<char*>(&pkt1));
				std::cout << "Assigned as client...\t";

				while (unfinishedPair.net[1]->packetReady == false) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				InitPacket* pkt2 = reinterpret_cast<InitPacket*>(&unfinishedPair.net[1]->packetData);
				if (pkt2->header.packetType != INIT_PACKET) {
					throw std::runtime_error("Incorrect Packet");
				}
				std::cout << "Assigned as pair...\t";
				memcpy(unfinishedPair.playerNames[1].data(), &(pkt2->header.pName), unfinishedPair.playerNames[1].size());


				pkt1.isHost = 0;
				memcpy(&pkt1.opName, unfinishedPair.playerNames[1].data(), unfinishedPair.playerNames[1].size());
				unfinishedPair.net[0]->sendPacket(reinterpret_cast<char*>(&pkt1));
				std::cout << "Replied to host...\t";
				//load hostname into memory;
				unfinishedPair.net[1]->packetReady = false;
				unfinishedPair.net[1]->packetFinished = true;
				unfinishedPair.complete = true;
				gamePairs.push_back(unfinishedPair);
				memcpy(&unfinishedPair, &nullPair, sizeof(NetworkServerPair)); //nullify the unfinished pair
				std::cout << "Pair added to stack" << std::endl;
			}
			qSocketReady = false;
			qSocketHandled = true;
		}
		//now enter the main loop, check each pair for commands.
		for (uint32_t i = 0; i < gamePairs.size(); i++) {
			
			//check host commands
			if (gamePairs[i].net[0]->packetReady == true) {
				std::cout << "Host command...\t";
				//read command, should be command packet
				CmdPacket* pkt1 = reinterpret_cast<CmdPacket*>(&gamePairs[i].net[0]->packetData);
				if (pkt1->header.packetType != CMD_PACKET) {
					std::cout << "Received erroneous packet, ignoring... " << std::endl;
				}
				else {
					if (pkt1->cmd[1] == '/') {
						//special command, check for newgame or disconnect
						if (memcmp(&pkt1->cmd[1], newgameChar.data(), newgameChar.size()) == 0) {
							//newgame command
							std::cout << "Newgame Command" << std::endl;
							handleNewgame(&gamePairs[i]);
						}
					}
					else {
						gamePairs[i].net[1]->sendPacket(&gamePairs[i].net[0]->packetData[0]); //send packet to client
						std::cout << "Sent to client" << std::endl;
					}
				}
				gamePairs[i].net[0]->packetReady = false;
				gamePairs[i].net[0]->packetFinished = true;
			}
			if (gamePairs[i].net[1]->packetReady == true) {
				//read command, should be command packet
				std::cout << "Client command...\t";
				CmdPacket* pkt1 = reinterpret_cast<CmdPacket*>(&gamePairs[i].net[1]->packetData);
				if (pkt1->header.packetType != CMD_PACKET) {
					std::cout << "Received erroneous packet, ignoring... " << std::endl;
				}
				else {
					std::cout << "Commmand Packet received, checking for special... ";
					if (pkt1->cmd[1] == '/') {
						//special command, check for newgame or disconnect
						if (memcmp(&pkt1->cmd[1], newgameChar.data(), newgameChar.size()) == 0) {
							//newgame command
							std::cout << "Newgame command" << std::endl;

							handleNewgame(&gamePairs[i]);
						}
					}
					else {
						std::cout << "Not special, relaying to host...";
						gamePairs[i].net[0]->sendPacket(&gamePairs[i].net[1]->packetData[0]); //send packet to client
						std::cout << "Sent to host" << std::endl;
					}
				}
				gamePairs[i].net[1]->packetReady = false;
				gamePairs[i].net[1]->packetFinished = true;
			}
			//now check both nets are active
			if (gamePairs[i].net[0]->sendShutdown == true && gamePairs[i].net[0]->connectionClosed == false) {
				//net 0 has not been shutdown, so cleanup net 0;
				gamePairs[i].net[0]->killWorker(); 
				std::cout << "Game " << i << " Net 0 dead ";
				//now check if close net 1 if not done already
				if (gamePairs[i].net[1]->connectionClosed == false) {
					gamePairs[i].net[1]->sendShutdown = true;
					gamePairs[i].net[1]->killWorker();
					std::cout << "Game " << i << " killing Net 1 ";
				}
			} //now do same for net 1
			else if (gamePairs[i].net[1]->sendShutdown == true && gamePairs[i].net[1]->connectionClosed == false) {
				//net 1 has not been shutdown, so cleanup net 1;
				gamePairs[i].net[1]->killWorker();
				std::cout << "Game " << i << " Net 1 dead ";
				//now check if close net 0 if not done already
				if (gamePairs[i].net[0]->connectionClosed == false) {
					gamePairs[i].net[0]->sendShutdown = true;
					gamePairs[i].net[0]->killWorker();
					std::cout << "Game " << i << " killing Net 0 ";
				}
			}
			//now if both are dead, erase from the gamePairs list
			if (gamePairs[i].net[0]->connectionClosed == true && gamePairs[i].net[1]->connectionClosed == true) {
				gamePairs.erase(gamePairs.begin() + i);
				std::cout << "Erased Game " << i << std::endl;
				i--;
			}
			//also check if current free net is near the end, if so loop back to beginning;
			if (currentFreeNet > 124) {
				currentFreeNet = 0;
			}
		}
	}
}
void NetworkingServer::handleNewgame(NetworkServerPair* pair) {
	pair->hostDealer = !pair->hostDealer;
	CmdPacket pkt1{};
	CmdPacket pkt2{};
	pkt1.header.packetType = CMD_PACKET;
	pkt2.header.packetType = CMD_PACKET;
	pkt1.cmd[0] = 0;
	pkt2.cmd[0] = 0;
	memcpy(&pkt1.cmd[1], newgameChar.data(), newgameChar.size());
	memcpy(&pkt2.cmd[1], newgameChar.data(), newgameChar.size());
	if (pair->hostDealer) {
		//send newgame0 to host, newgame1 to client, wait for stock from host and sent to client

		pkt1.cmd[9] = '0';
		pkt2.cmd[9] = '1';
	}
	else {
		pkt1.cmd[9] = '1';
		pkt2.cmd[9] = '0';
	}

	pair->net[0]->sendPacket(reinterpret_cast<char*>(&pkt1));
	std::cout << "Sending command... ";
	for (char i = 0; i < 32; i++) {
		std::cout << pkt1.cmd[i];
	}
	std::cout << std::endl;
	pair->net[1]->sendPacket(reinterpret_cast<char*>(&pkt2));
	std::cout << "Sending command... ";
	for (char i = 0; i < 32; i++) {
		std::cout << pkt2.cmd[i];
	}
	std::cout << std::endl;

	pair->net[0]->packetReady = false;
	pair->net[0]->packetFinished = true;
	while (pair->net[0]->packetReady == false) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	GamePacket* sPtr = reinterpret_cast<GamePacket*>(&pair->net[0]->packetData);
	if (sPtr->header.packetType != GAME_PACKET) {
		throw std::runtime_error("Expected to receive stock");
	}
	memcpy(&pair->initialStock[0], &sPtr->cardData, 52 * sizeof(playingCard));
	pair->net[1]->sendPacket(&pair->net[0]->packetData[0]);
	pair->net[0]->packetReady = false;
	pair->net[0]->packetFinished = true;

}





//	while (gameRunning) {
//		int iRemoteAddrLen = sizeof(sockaddr_in);
//		cSockets.push_back(accept(lSocket, (sockaddr*)&remAddr1, &iRemoteAddrLen));
//		std::cout << "Connection being handled" << std::endl;
//		if (cSockets.back() == INVALID_SOCKET) {
//			closesocket(lSocket);
//			WSACleanup();
//			throw std::runtime_error("Failed to accept socket");
//		}
//		//closesocket(lSocket);
//		threads.push_back(std::thread(&NetworkingServer::handleConnection, this, cSockets.back(), currentusrs));
//	}
//
//	// cleanup
//	for (uint32_t i = 0; i < threads.size(); i++) {
//		threads[i].join();
//	}
//	closesocket(lSocket);
//	WSACleanup();
//
//
//}
//void NetworkingServer::handleConnection(SOCKET s, int index) {
//	std::cout << index << " Handling Connection" << std::endl;
//	char initbuf[INITIAL_PKTLEN];
//	readPacket(s, &initbuf[0], INITIAL_PKTLEN);
//	sharedMutex.lock();
//	std::array<char, 8> usrn;
//	memcpy(usrn.data(), &initbuf[0], usrn.size() * sizeof(usrn[0]));
//	usrs.push_back(usrn);
//	currentusrs++;
//	sharedMutex.unlock();
//	uint32_t serverConfig = 0;
//	if (currentusrs == 1) {
//		std::cout << "Setting Host" << std::endl;
//		serverConfig |= SVRCNF_HOST_BIT;
//		memset(&initbuf[0], 0, INITIAL_PKTLEN);
//		memcpy(&initbuf[8], &serverConfig, sizeof(serverConfig));
//		int sResult = send(s, &initbuf[0], INITIAL_PKTLEN, 0);
//		if (sResult == SOCKET_ERROR) {
//			printf("send failed with error: %d\n", WSAGetLastError());
//			closesocket(s);
//			WSACleanup();
//			throw std::runtime_error("Failed to send packet");
//		}
//		while (correctPlayerCount == false) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		}
//		std::cout << "Confirming Opponent" << std::endl;
//		memcpy(&initbuf[0], &usrs[1], INITIAL_PKTLEN);
//		memcpy(&initbuf[8], &serverConfig, sizeof(serverConfig));
//		sResult = send(s, &initbuf[0], INITIAL_PKTLEN, 0);
//		if (sResult == SOCKET_ERROR) {
//			printf("send failed with error: %d\n", WSAGetLastError());
//			closesocket(s);
//			WSACleanup();
//			throw std::runtime_error("Failed to send packet");
//		}
//		char stockBuf[STOCK_PKTLEN];
//		readPacket(s, &stockBuf[0], STOCK_PKTLEN);
//		stock.resize(52);
//		memcpy(stock.data(), &stockBuf[0], STOCK_PKTLEN);
//		stockRecv = true;
//		std::cout << "Recevied Stock" << std::endl;
//	}
//	else if (currentusrs == 2) {
//		std::cout << "Confirming Client" << std::endl;
//		correctPlayerCount = true;
//		serverConfig |= SVRCNF_CLIENT_BIT;
//		memcpy(&initbuf[0], &usrs[0], INITIAL_PKTLEN);
//		memcpy(&initbuf[8], &serverConfig, sizeof(serverConfig));
//		int sResult = send(s, &initbuf[0], INITIAL_PKTLEN, 0);
//		if (sResult == SOCKET_ERROR) {
//			printf("send failed with error: %d\n", WSAGetLastError());
//			closesocket(s);
//			WSACleanup();
//			throw std::runtime_error("Failed to send packet");
//		}
//		while (stockRecv == false) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		}
//		std::cout << "Sending Stock" << std::endl;
//		sResult = send(s, (char*)stock.data(), STOCK_PKTLEN, 0);
//		if (sResult == SOCKET_ERROR) {
//			printf("send failed with error: %d\n", WSAGetLastError());
//			closesocket(s);
//			WSACleanup();
//			throw std::runtime_error("Failed to send packet");
//		}
//	}
//	while (gameRunning) {
//		if (index == playerTurn) {
//			std::cout << "Waiting for Command" << std::endl;
//			readPacket(s, command.data(), CMD_PKTLEN);
//			commandReady = true;
//			std::string str(command.begin() + 1, command.end());
//			if (str == "/disconnect") {
//				gameRunning = false;
//			}
//			while (commandReady == true) {
//				std::this_thread::sleep_for(std::chrono::milliseconds(10));
//			}
//		}
//		else {
//			while (commandReady == false) {
//				std::this_thread::sleep_for(std::chrono::milliseconds(10));
//			}
//			int sResult = send(s, command.data(), CMD_PKTLEN, 0);
//			std::cout << "Sent Command" << std::endl;
//			if (sResult == SOCKET_ERROR) {
//				printf("send failed with error: %d\n", WSAGetLastError());
//				closesocket(s);
//				WSACleanup();
//				throw std::runtime_error("Failed to send packet");
//			}
//			playerTurn = (playerTurn + 1) % 2;
//			commandReady = false;
//		}
//	}
//	sharedMutex.lock();
//	currentusrs--;
//	sharedMutex.unlock();
//	closesocket(s);
//}


//
//void NetworkingServer::readPacket(SOCKET s, char* buf, size_t len) {
//	int receivedDataLen = 0;
//	size_t cursor = 0;
//	int rResult = 0;
//	char recvbuf[DEFAULT_BUFLEN];
//
//	do
//	{
//		rResult = recv(s, recvbuf, DEFAULT_BUFLEN, 0);
//		if (rResult > 0) {
//			//take data out of recvbuf
//			memcpy(buf + cursor, recvbuf, rResult);
//			cursor += rResult;
//			receivedDataLen += rResult;
//		}
//		else if (rResult == 0) {
//			std::cout << "Connection Closed" << std::endl;
//			return;
//		}
//		else {
//			
//			closesocket(s);
//			WSACleanup();
//			throw std::runtime_error("Failed receive error socket");
//		}
//	} while (receivedDataLen < len);
//}