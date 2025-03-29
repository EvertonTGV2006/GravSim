#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
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
			if (nextBlockSize == 0) {
				stat->addMessage(MSG_LEVEL_NETWORK_LOW, "Recieved kill packet");
				sendShutdown = true; break;
			}

			errCode = readDataBlock(nextBlockSize, &packetData[0] + sizeof(HeaderData));
			if (errCode == 1) { sendShutdown = true; break; }

			packetFinished = false;
			//std::cout << "Received packet of size " << nextBlockSize + sizeof(HeaderData) << std::endl;
			stat->addMessage(MSG_LEVEL_NETWORK_LOW, "Received packet of size " + std::to_string(nextBlockSize + sizeof(HeaderData)));
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
		//std::cout << "Sent packet of size " << packetSize << " with x bytes sent " << iResult << std::endl;
		stat->addMessage(MSG_LEVEL_NETWORK_LOW, "Sent packet of size " + std::to_string(packetSize));
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
	case KILL_PACKET:
		return 0;
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
void NetworkUtil::startWorker(SOCKET* sockPtr, StatusLogger* statPtr) {
	packetReady = false;
	packetFinished = true;
	sendShutdown = false;
	shutdownSent = false;
	closeSocket = false;
	sock = sockPtr;
	stat = statPtr;

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


void NetworkingClient::initWinsock(PlayerDetails* initDets) {
	//std::cout << "Initialisng networking..." << std::endl;
	stat->addMessage(MSG_LEVEL_NETWORK_MID, "Initialising Networking");
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

	//std::cout << "Attempting to connect to " << ipaddr << " on port " << portaddr << std::endl;
	stat->addMessage(MSG_LEVEL_NETWORK_HIGH, "Connecting to " + ipaddr + " on port " + std::to_string(portaddr));

	if (connect(hSocket, (sockaddr*)(&sockAddr1), sizeof(sockAddr1)) != 0)
	{
		stat->addMessage(MSG_LEVEL_USER, "Failed to connect to server");
		throw std::runtime_error("Failed to connect socket");
	}
	stat->addMessage(MSG_LEVEL_USER, "Connected to server successfully");
	//std::cout << "Connected to Server as ";
	//for (size_t i = 0; i < usrn.size(); i++) {
	//	std::cout << usrn[i];
	//}
	//std::cout << std::endl;

	//now start worker thread
	net.startWorker(&hSocket, stat);

	//now send packet with local player details
	InitPacket pkt1{};
	pkt1.header.packetType = INIT_PACKET;
	pkt1.dets[0] = *initDets;
	locPlayerDetails = *initDets;

	net.sendPacket(reinterpret_cast<char*>(&pkt1));
	//std::cout << "Sent greeting packet, waiting for reply... ";
	stat->addMessage(MSG_LEVEL_NETWORK_MID, "Sent greeting packet, waiting for opponent...");
	// 
	////now we wait for a reply...
	//while (net.packetReady == false) {
	//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	//}
	////std::cout << "Recevied reply" << std::endl;
	////stat->addMessage(MSG_LEVEL_NETWORK_MID, "Recieved reply");
	////now we parse reply;
	//InitPacket* pkt2 = reinterpret_cast<InitPacket*>(&net.packetData);
	//if (pkt2->header.packetType != INIT_PACKET) {
	//	stat->addMessage(MSG_LEVEL_URGENT, "Received incorrect packet, exiting");
	//	//std::cout << "Received incorrect packet, exiting" << std::endl;
	//	throw std::runtime_error("Incorrect Packet");
	//}


	//stat->addMessage(MSG_LEVEL_NETWORK_HIGH, "Selected as game host");


	//net.packetReady = false;
	//net.packetFinished = true;

	//if (isGameHost) {


	//	//if game host, we wait for another greeting packet
	//	//std::cout << "Selected as game host, waiting for opponent... ";
	//	stat->addMessage(MSG_LEVEL_USER, "Waiting for opponent...");
	//	
	//	while (net.packetReady == false) {
	//		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	//	}

	//	if (pkt2->header.packetType != INIT_PACKET) {
	//		stat->addMessage(MSG_LEVEL_URGENT, "Received incorrect packet, exiting");
	//		//std::cout << "Received incorrect packet, exiting" << std::endl;
	//		throw std::runtime_error("Incorrect Packet");
	//	}
	//	//parse new packet
	//	/*std::cout << "Opponent Found: ";*/
	//	stat->addMessage(MSG_LEVEL_USER,"Opponent Found");

	//	memcpy(oppn.data(), &(pkt2->opName), oppn.size());
	//	//for (uint32_t i = 0; i < oppn.size(); i++) {
	//	//	std::cout << oppn[i];
	//	//}
	//	//std::cout << std::endl;
	//	net.packetReady = false;
	//	net.packetFinished = true;

	//	//function sends a newgame command
	//	CmdPacket pkt3{};
	//	pkt3.header.packetType = CMD_PACKET;
	//	memcpy(&pkt3.header.pName, usrn.data(), usrn.size());
	//	std::array<char, 9> cmd = { 0,'/','n','e','w','g','a','m','e' };
	//	memcpy(&pkt3.cmd, cmd.data(), cmd.size());
	//	net.sendPacket(reinterpret_cast<char*>(&pkt3));

	//}
	//else {

	//	/*std::cout << "Not hosting game, opponent found: ";*/
	//	stat->addMessage(MSG_LEVEL_USER, "Opponent Found");
	//	memcpy(oppn.data(), &(pkt2->opName), oppn.size());
	//	//for (uint32_t i = 0; i < oppn.size(); i++) {
	//	//	std::cout << oppn[i];
	//	//}
	//	//std::cout << std::endl;
	//	net.packetReady = false;
	//	net.packetFinished = true;
	//}
	//


	////GamePacket* pkt3{};
	////std::cout << "Waiting to receive game details... ";
	////while (net.packetReady == false) {
	////	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	////}
	////std::cout << "Recveived game details" << std::endl;
	////pkt3 = reinterpret_cast<GamePacket*>(&net.packetData);
	////if (pkt3->header.packetType != GAME_PACKET) {
	////	std::cout << "Received incorrect packet, exiting" << std::endl;
	////	throw std::runtime_error("Incorrect Packet");
	////}
	////stockPtr->resize(52);
	////memcpy(stockPtr->data(), pkt3->cardData, stockPtr->size());
	////net.packetReady = false;
	////net.packetFinished = true;

}
bool NetworkingClient::recvInitPacket() {
	bool dealRequired = false;
	InitPacket* pkt = reinterpret_cast<InitPacket*>(&net.packetData);
	dets = pkt->dets;

	std::stringstream ss;
	for (uint32_t i = 0; i < locPlayerDetails.name.size(); i++) {
		ss << locPlayerDetails.name[i];
		std::cout << locPlayerDetails.name[i];
	}std::cout << std::endl;
	for (uint32_t i = 0; i < locPlayerDetails.name.size(); i++) {
		std::cout << dets[0].name[i];
	}std::cout << std::endl;
	for (uint32_t i = 0; i < locPlayerDetails.name.size(); i++) {
		std::cout << dets[1].name[i];
	}std::cout << std::endl;
	stat->addMessage(MSG_LEVEL_USER, ss.str());

	if (locPlayerDetails.name == dets[0].name) {
		locPlayerIndex = 0;
		dealRequired = true;
		net.packetReady = false;
		net.packetFinished = true;
		
		std::cout << "DEALER" << std::endl;
	}
	else if (locPlayerDetails.name == dets[1].name) {
		locPlayerIndex = 1;
		dealRequired = false;
		net.packetReady = false;
		net.packetFinished = true;
		stat->addMessage(MSG_LEVEL_USER, "Waiting to receive game deck");
		while (net.packetReady == false) {//block until deal is received
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	else {
		std::cout << "INIT PACKET BRANCH FALLTHROUGH";
		stat->addMessage(MSG_LEVEL_URGENT, "INIT PACKET FALLTHROUGH");
	}
	return dealRequired;
}

void NetworkingClient::cleanup() {
	/*std::cout << "Cleaning up networking... " << std::endl;*/
	stat->addMessage(MSG_LEVEL_NETWORK_HIGH, "Cleaning up networking");
	HeaderData kp{};
	kp.packetType = KILL_PACKET;
	net.sendPacket(reinterpret_cast<char*>(&kp));
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

	//std::cout << "Listenting" << std::endl;
	stat->addMessage(MSG_LEVEL_STARTUP, "Listening on port 25566");

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
			//std::cout << WSAGetLastError() << std::endl;
			//std::cout << "Error accepting, invalid socket, ignoring... " << std::endl;
			stat->addMessage(MSG_LEVEL_URGENT, "Error accepting invalid socket, ignoring error " + std::to_string(WSAGetLastError()));
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
			//std::cout << "Incoming socket...\t";
			stat->addMessage(MSG_LEVEL_NETWORK_HIGH, "Incoming socket connection...");
			if (unfinishedPair.partComplete == false) {
				unfinishedPair.net[0] = &nets[currentFreeNet];
				unfinishedPair.net[0]->startWorker(&socks[currentFreeNet], stat);
				unfinishedPair.partComplete = true;
			}
			else {
				unfinishedPair.net[1] = &nets[currentFreeNet];
				unfinishedPair.net[1]->startWorker(&socks[currentFreeNet], stat);
				unfinishedPair.complete = true;
			}
			currentFreeNet++;
				
			if (unfinishedPair.complete == false) {
				//receive hostname and store


				//wait for incoming host packet
				while (unfinishedPair.net[0]->packetReady == false) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					if (unfinishedPair.net[0]->sendShutdown) {
						break;
					}
				}
				InitPacket* pkt2 = reinterpret_cast<InitPacket*>(&unfinishedPair.net[0]->packetData);
				if (pkt2->header.packetType != INIT_PACKET) {
					throw std::runtime_error("Incorrect Packet");
				}
				//std::cout << "Received Name\t" << std::endl;
				unfinishedPair.dets[0] = pkt2->dets[0];
				//load hostname into memory;
				unfinishedPair.net[0]->packetReady = false;
				unfinishedPair.net[0]->packetFinished = true;
			}
			else {
				//recieve hostname and store, then send reply init packets
				while (unfinishedPair.net[1]->packetReady == false) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				InitPacket* pkt2 = reinterpret_cast<InitPacket*>(&unfinishedPair.net[1]->packetData);
				if (pkt2->header.packetType != INIT_PACKET) {
					throw std::runtime_error("Incorrect Packet");
				}
				unfinishedPair.dets[1] = pkt2->dets[0];

				unfinishedPair.net[1]->packetReady = false;
				unfinishedPair.net[1]->packetFinished = true;
				unfinishedPair.sendInitPackets = true;
				unfinishedPair.gameNumber = 0;
				unfinishedPair.complete = true;
				gamePairs.push_back(unfinishedPair);

				if (unfinishedPair.net[0]->sendShutdown == true) {
					gamePairs[gamePairs.size() - 1].exitRequired = true;
				}




				memcpy(&unfinishedPair, &nullPair, sizeof(NetworkServerPair)); //nullify the unfinished pair
				//std::cout << "Pair added to stack" << std::endl;
				stat->addMessage(MSG_LEVEL_NETWORK_HIGH, "Game pair added at index " + std::to_string(gamePairs.size() - 1));
			}
			qSocketReady = false;
			qSocketHandled = true;
		}
		//now enter the main loop, check each pair for commands.
		for (uint32_t i = 0; i < gamePairs.size(); i++) {

			if (gamePairs[i].sendInitPackets == true) {
				//starting a newgame, send inital packets then wait for game packet from host to transfer on;
				//first work out playerIDs
				gamePairs[i].dets[0].playerID = gamePairs[i].gameNumber % 2;
				gamePairs[i].dets[1].playerID = (gamePairs[i].gameNumber + 1) % 2;

				InitPacket pkt1{};
				pkt1.header.packetType = INIT_PACKET;
				pkt1.header.globalID = 0;
				pkt1.dets = gamePairs[i].dets;
				gamePairs[i].net[0]->sendPacket(reinterpret_cast<char*>(&pkt1));
				gamePairs[i].net[1]->sendPacket(reinterpret_cast<char*>(&pkt1));

				//now wait for game packet and pass on to other player
				if (gamePairs[i].dets[0].playerID == 0) {
					while (gamePairs[i].net[0]->packetReady == false) {
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					gamePairs[i].net[1]->sendPacket(&gamePairs[i].net[0]->packetData[0]);
					gamePairs[i].net[0]->packetReady = false;
					gamePairs[i].net[0]->packetFinished = true;
				}
				else if (gamePairs[i].dets[1].playerID == 0) {
					while (gamePairs[i].net[1]->packetReady == false) {
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					gamePairs[i].net[0]->sendPacket(&gamePairs[i].net[1]->packetData[0]);
					gamePairs[i].net[1]->packetReady = false;
					gamePairs[i].net[1]->packetFinished = true;
				}
				gamePairs[i].sendInitPackets = false;
			}


			//now handle normal commands
			//just check the command and relay if ok
			if (gamePairs[i].net[0]->packetReady == true) {
				HeaderData* hd = reinterpret_cast<HeaderData*>(&gamePairs[i].net[0]->packetData);
				if (hd->packetType != CMD_PACKET) {
					stat->addMessage(MSG_LEVEL_URGENT, "Received erroneus packet from net 0, killing game " + std::to_string(i));
					std::stringstream ss;
					std::stringstream st;
					for (uint32_t j = 0; j < MAX_PKT_SIZE; j++) {
						ss << uint32_t(gamePairs[i].net[0]->packetData[j]) << " ";
						st << gamePairs[i].net[0]->packetData[j] << ' ';
					}
					stat->addMessage(MSG_LEVEL_NETWORK_LOW, ss.str() + '\n' + st.str());
					gamePairs[i].exitRequired = true;
				}
				else {
					if (checkCommand(&gamePairs[i], 0)) {
						gamePairs[i].net[1]->sendPacket(&gamePairs[i].net[0]->packetData[0]);
						gamePairs[i].net[0]->packetFinished = true;
						gamePairs[i].net[0]->packetReady = false;
					}
				}
			}
			if (gamePairs[i].net[1]->packetReady == true) {
				HeaderData* hd = reinterpret_cast<HeaderData*>(&gamePairs[i].net[1]->packetData);
				if (hd->packetType != CMD_PACKET) {
					stat->addMessage(MSG_LEVEL_URGENT, "Received erroneus packet from net 1, killing game " + std::to_string(i));
					std::stringstream ss;
					std::stringstream st;
					for (uint32_t j = 0; j < MAX_PKT_SIZE; j++) {
						ss << uint32_t(gamePairs[i].net[1]->packetData[j]) << " ";
						st << gamePairs[i].net[1]->packetData[j] << ' ';
					}
					stat->addMessage(MSG_LEVEL_NETWORK_LOW, ss.str() + '\n' + st.str());
					gamePairs[i].exitRequired = true;
				}
				else {
					if (checkCommand(&gamePairs[i], 1)) {
						gamePairs[i].net[0]->sendPacket(&gamePairs[i].net[1]->packetData[0]);
						gamePairs[i].net[1]->packetFinished = true;
						gamePairs[i].net[1]->packetReady = false;
					}
				}
			}

			//kill net if exit required
			if (gamePairs[i].exitRequired == true) {
				gamePairs[i].net[0]->killWorker();
				gamePairs[i].net[1]->killWorker();
			}

			//now check both nets are active
			if (gamePairs[i].net[0]->sendShutdown == true && gamePairs[i].net[0]->connectionClosed == false) {
				//net 0 has not been shutdown, so cleanup net 0;
				gamePairs[i].net[0]->killWorker(); 
				//std::cout << "Game " << i << " Net 0 dead ";
				//now check if close net 1 if not done already
				if (gamePairs[i].net[1]->connectionClosed == false) {
					gamePairs[i].net[1]->sendShutdown = true;
					gamePairs[i].net[1]->killWorker();
					//std::cout << "Game " << i << " killing Net 1 ";
				}
			} //now do same for net 1
			else if (gamePairs[i].net[1]->sendShutdown == true && gamePairs[i].net[1]->connectionClosed == false) {
				//net 1 has not been shutdown, so cleanup net 1;
				gamePairs[i].net[1]->killWorker();
				//std::cout << "Game " << i << " Net 1 dead ";
				//now check if close net 0 if not done already
				if (gamePairs[i].net[0]->connectionClosed == false) {
					gamePairs[i].net[0]->sendShutdown = true;
					gamePairs[i].net[0]->killWorker();
					//std::cout << "Game " << i << " killing Net 0 ";
				}
			}
			//now if both are dead, erase from the gamePairs list
			if (gamePairs[i].net[0]->connectionClosed == true && gamePairs[i].net[1]->connectionClosed == true) {
				gamePairs.erase(gamePairs.begin() + i);
				stat->addMessage(MSG_LEVEL_NETWORK_HIGH, "Erased Game " + std::to_string(i));
				//std::cout << "Erased Game " << i << std::endl;
				i--;
			}
			//also check if current free net is near the end, if so loop back to beginning;
			if (currentFreeNet > 124) {
				currentFreeNet = 0;
			}
		}
	}
}
bool NetworkingServer::checkCommand(NetworkServerPair* gamePair, uint32_t cmdIndex) {
	bool relayCmd = true;

	CmdPacket* pkt = reinterpret_cast<CmdPacket*>(&gamePair->net[cmdIndex]->packetData);
	std::string cmd;
	cmd.resize(30);
	memcpy(cmd.data(), &pkt->cmd, 30 * sizeof(char));
	if (cmd.find("/newgame") != std::string::npos) {
		gamePair->sendInitPackets = true;
		relayCmd = false;
	}
	else if (cmd.find("/kill") != std::string::npos || cmd.find("/exit") != std::string::npos) {
		gamePair->exitRequired = true;
		relayCmd = false;
	}
	return relayCmd;
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