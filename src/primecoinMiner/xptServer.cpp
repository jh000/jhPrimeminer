#include"global.h"

/*
 * Creates a new x.pushthrough server instance that listens on the specified port
 */
xptServer_t* xptServer_create(uint16 port)
{
	xptServer_t* xptServer = (xptServer_t*)malloc(sizeof(xptServer_t));
	memset(xptServer, 0x00, sizeof(xptServer_t));
	// open port for incoming connections
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=ADDR_ANY;
	if( bind(s,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN)) == SOCKET_ERROR )
	{
		closesocket(s);
		free(xptServer);
		return NULL;
	}
	listen(s, 64);
	xptServer->acceptSocket = s;
	// init client list
	xptServer->list_connections = simpleList_create(64);
	// return server object
	return xptServer;
}

/*
 * Initializes a new client instance with the server
 */
xptServerClient_t* xptServer_newClient(xptServer_t* xptServer, SOCKET s)
{
	xptServerClient_t* xptServerClient = (xptServerClient_t*)malloc(sizeof(xptServerClient_t));
	memset(xptServerClient, 0x00, sizeof(xptServerClient_t));
	// setup client data
	xptServerClient->clientSocket = s;
	xptServerClient->packetbuffer = xptPacketbuffer_create(4*1024); // 4kb
	xptServerClient->xptServer = xptServer;
	// set socket as non-blocking
	unsigned int nonblocking=1;
	unsigned int cbRet;
	WSAIoctl(s, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);
	// return client object
	return xptServerClient;
}

/*
 * Called whenever we received a full packet from a client
 * Return false if the packet is invalid and the client should be disconnected
 */
bool xptServer_processPacket(xptServer_t* xptServer, xptServerClient_t* xptServerClient)
{
	if( xptServerClient->opcode == XPT_OPC_C_AUTH_REQ )
	{
		return xptServer_processPacket_authRequest(xptServer, xptServerClient);
	}
	printf("xptServer_processPacket(): Received unknown opcode %d\n", xptServerClient->opcode);
	return false; // invalid packet -> disconnect client
}

/*
 * Called whenever we received some bytes from a client
 */
bool xptServer_receiveData(xptServer_t* xptServer, xptServerClient_t* xptServerClient)
{
	sint32 packetFullSize = 4; // the packet always has at least the size of the header
	if( xptServerClient->recvSize > 0 )
		packetFullSize += xptServerClient->recvSize;
	sint32 bytesToReceive = (sint32)(packetFullSize - xptServerClient->recvIndex);
	// packet buffer is always large enough at this point
	sint32 r = recv(xptServerClient->clientSocket, (char*)(xptServerClient->packetbuffer->buffer+xptServerClient->recvIndex), bytesToReceive, 0);
	if( r <= 0 )
	{
		// receive error, client disconnected
		return false;
	}
	xptServerClient->recvIndex += r;
	// header just received?
	if( xptServerClient->recvIndex == packetFullSize && packetFullSize == 4 )
	{
		// process header
		uint32 headerVal = *(uint32*)xptServerClient->packetbuffer->buffer;
		uint32 opcode = (headerVal&0xFF);
		uint32 packetDataSize = (headerVal>>8)&0xFFFFFF;
		// validate header size
		if( packetDataSize >= (1024*1024*2-4) )
		{
			// packets larger than 4mb are not allowed
			printf("xptServer_receiveData(): Packet exceeds 2mb size limit\n");
			return false;
		}
		xptServerClient->recvSize = packetDataSize;
		xptServerClient->opcode = opcode;
		// enlarge packetBuffer if too small
		if( (xptServerClient->recvSize+4) > xptServerClient->packetbuffer->bufferLimit )
		{
			xptPacketbuffer_changeSizeLimit(xptServerClient->packetbuffer, (xptServerClient->recvSize+4));
		}
	}
	// have we received the full packet?
	if( xptServerClient->recvIndex >= (xptServerClient->recvSize+4) )
	{
		// process packet
		if( xptServer_processPacket(xptServer, xptServerClient) == false )
			return false;
		xptServerClient->recvIndex = 0;
		xptServerClient->opcode = 0;
	}
	return true;
}

/*
 * Deletes the client and frees the associated client data
 * Note that this method should never be called directly, if you want to disconnect a client - set xptServerClient->disconnected = true
 */
void xptServer_deleteClient(xptServer_t* xptServer, xptServerClient_t* xptServerClient)
{
	if( xptServerClient->packetbuffer )
		xptPacketbuffer_free(xptServerClient->packetbuffer);
	free(xptServerClient);
}

/*
 * Sends new block data to each client
 */
void xptServer_sendNewBlockToAll(xptServer_t* xptServer, uint32 coinTypeIndex)
{
	uint32 time1 = GetTickCount();
	sint32 workerCount = 0;
	sint32 payloadCount = 0;
	for(uint32 i=0; i<xptServer->list_connections->objectCount; i++)
	{
		xptServerClient_t* xptServerClient = (xptServerClient_t*)xptServer->list_connections->objects[i];
		if( xptServerClient->disconnected || xptServerClient->clientState != XPT_CLIENT_STATE_LOGGED_IN )
			continue;
		if( xptServerClient->coinTypeIndex != coinTypeIndex )
			continue;
		// send block data
		xptServer_sendBlockData(xptServer, xptServerClient);
		workerCount++;
		payloadCount += xptServerClient->payloadNum;
	}
	uint32 time2 = GetTickCount() - time1;
	printf("Send %d blocks to %d workers in %dms\n", payloadCount, workerCount, time2);
}

/*
 * Checks for new blocks that should be propagated
 */
void xptServer_checkForNewBlocks(xptServer_t* xptServer)
{
	uint32 numberOfCoinTypes = 0;
	uint32 blockHeightPerCoinType[32] = {0};
	xptServer->xptCallback_getBlockHeight(xptServer, &numberOfCoinTypes, blockHeightPerCoinType);
	for(uint32 i=0; i<numberOfCoinTypes; i++)
	{
		if( blockHeightPerCoinType[i] > xptServer->coinTypeBlockHeight[i] )
		{
			printf("New block arrived for coinType %d\n", i+1);
			xptServer_sendNewBlockToAll(xptServer, i);
			xptServer->coinTypeBlockHeight[i] = blockHeightPerCoinType[i];
		}
	}
}

/*
 * Starts processing 
 */
void xptServer_startProcessing(xptServer_t* xptServer)
{
	FD_SET fd;
	timeval sTimeout;
	sTimeout.tv_sec = 0;
	sTimeout.tv_usec = 250000;
	//Sleep(1000); // 3 seconds pause to make sure other stuff has inited (there seems to be a crashbug somewhere?)
	while( true )
	{
		FD_ZERO(&fd);
		// add server accept socket
		FD_SET(xptServer->acceptSocket, &fd); // this line crashes?
		// add all connected sockets
		for(uint32 i=0; i<xptServer->list_connections->objectCount; i++)
		{
			xptServerClient_t* client = (xptServerClient_t*)simpleList_get(xptServer->list_connections, i);
			FD_SET(client->clientSocket, &fd);
		}
		// check for socket events
		sint32 r = select(0, &fd, 0, 0, &sTimeout); // wait one second
		if( r )
		{
			// check for new connections
			if( FD_ISSET(xptServer->acceptSocket, &fd) )
			{
				xptServer_newClient(xptServer, accept(xptServer->acceptSocket, 0, 0));
				continue; // todo: this causes a bug without continue?
			}
			// check for client data received
			for(uint32 i=0; i<xptServer->list_connections->objectCount; i++)
			{
				xptServerClient_t* client = (xptServerClient_t*)simpleList_get(xptServer->list_connections, i);
				if( FD_ISSET(client->clientSocket, &fd) )
				{
					if( xptServer_receiveData(xptServer, client) == false )
					{
						// something went wrong inside of _receiveData()
						// make sure the client gets disconnected
						client->disconnected = true;
						closesocket(client->clientSocket);
						client->clientSocket = 0;
					}
				}
				if( client->disconnected )
				{
					// close socket if not already done
					if( client->clientSocket != 0 )
					{
						closesocket(client->clientSocket);
						client->clientSocket = 0;
					}
					// delete client
					xptServer_deleteClient(xptServer, client);
					// remove from list
					xptServer->list_connections->objects[i] = xptServer->list_connections->objects[xptServer->list_connections->objectCount-1];
					xptServer->list_connections->objectCount--;
					i--;
				}
			}
		}
		else
			Sleep(1);
		// check for new blocks
		xptServer_checkForNewBlocks(xptServer);


		// check all clients for connection blocking
		// other stuff
		//client->connectionOpenedTimer = GetTickCount();
		//uint32 connectionTimeout = 35*1000;
		//uint32 currentTimeoutTimer = GetTickCount();
		//for(uint32 i=0; i<jrs->list_connections->objectCount; i++)
		//{
		//	xptServerClient_t* client = (xptServerClient_t*)simpleList_get(jrs->list_connections, i);
		//	if( currentTimeoutTimer > (client->connectionOpenedTimer+connectionTimeout) )
		//	{
		//		// delete client
		//		jsonRpc_deleteClient(client);
		//		// remove from list
		//		jrs->list_connections->objects[i] = jrs->list_connections->objects[jrs->list_connections->objectCount-1];
		//		jrs->list_connections->objectCount--;
		//		i--;
		//	}
		//}


	}
}