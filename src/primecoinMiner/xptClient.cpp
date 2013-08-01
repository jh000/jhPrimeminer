#include"global.h"


SOCKET xptClient_openConnection(char *IP, int Port)
{
	SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(Port);
	addr.sin_addr.s_addr=inet_addr(IP);
	int result = connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN));
	if( result )
	{
		return 0;
	}
	return s;
}

/*
 * Opens a new x.pushthrough connection
 * target is the server address + worker login data to use for connecting
 */
xptClient_t* xptClient_connect(jsonRequestTarget_t* target, uint32 payloadNum)
{
	// first try to connect to the given host/port
	SOCKET clientSocket = xptClient_openConnection(target->ip, target->port);
	if( clientSocket == 0 )
		return NULL;
	// set socket as non-blocking
	unsigned int nonblocking=1;
	unsigned int cbRet;
	WSAIoctl(clientSocket, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);
	// initialize the client object
	xptClient_t* xptClient = (xptClient_t*)malloc(sizeof(xptClient_t));
	memset(xptClient, 0x00, sizeof(xptClient_t));
	xptClient->clientSocket = clientSocket;
	xptClient->sendBuffer = xptPacketbuffer_create(64*1024);
	xptClient->recvBuffer = xptPacketbuffer_create(64*1024);
	fStrCpy(xptClient->username, target->authUser, 127);
	fStrCpy(xptClient->password, target->authPass, 127);
	xptClient->payloadNum = max(1, min(127, payloadNum));
	InitializeCriticalSection(&xptClient->cs_shareSubmit);
	xptClient->list_shareSubmitQueue = simpleList_create(4);
	// send worker login
	xptClient_sendWorkerLogin(xptClient);
	// return client object
	return xptClient;
}

/*
 * Disconnects and frees the xptClient instance
 */
void xptClient_free(xptClient_t* xptClient)
{
	xptPacketbuffer_free(xptClient->sendBuffer);
	xptPacketbuffer_free(xptClient->recvBuffer);
	if( xptClient->clientSocket != 0 )
	{
		closesocket(xptClient->clientSocket);
	}
	simpleList_free(xptClient->list_shareSubmitQueue);
	free(xptClient);
}

/*
 * Sends the worker login packet
 */
void xptClient_sendWorkerLogin(xptClient_t* xptClient)
{
	uint32 usernameLength = min(127, fStrLen(xptClient->username));
	uint32 passwordLength = min(127, fStrLen(xptClient->password));
	// build the packet
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptClient->sendBuffer, XPT_OPC_C_AUTH_REQ);
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, 2);								// version
	xptPacketbuffer_writeString(xptClient->sendBuffer, xptClient->username, 128, &sendError);	// username
	xptPacketbuffer_writeString(xptClient->sendBuffer, xptClient->password, 128, &sendError);	// password
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptClient->payloadNum);			// payloadNum
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptClient->sendBuffer);
	// send to client
	send(xptClient->clientSocket, (const char*)(xptClient->sendBuffer->buffer), xptClient->sendBuffer->parserIndex, 0);
}

/*
 * Sends the share packet
 */
void xptClient_sendShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit)
{
	//printf("Send share\n");
	// build the packet
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptClient->sendBuffer, XPT_OPC_C_SUBMIT_SHARE);
	xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->merkleRoot, 32, &sendError);		// merkleRoot
	xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->prevBlockHash, 32, &sendError);	// prevBlock
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->version);				// version
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->nTime);				// nTime
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->nonce);				// nNonce
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->nBits);				// nBits
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->sieveSize);			// sieveSize
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->sieveCandidate);		// sieveCandidate
	// bnFixedMultiplier
	xptPacketbuffer_writeU8(xptClient->sendBuffer, &sendError, xptShareToSubmit->fixedMultiplierSize);
	xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->fixedMultiplier, xptShareToSubmit->fixedMultiplierSize, &sendError);
	// bnChainMultiplier
	xptPacketbuffer_writeU8(xptClient->sendBuffer, &sendError, xptShareToSubmit->chainMultiplierSize);
	xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->chainMultiplier, xptShareToSubmit->chainMultiplierSize, &sendError);
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptClient->sendBuffer);
	// send to client
	send(xptClient->clientSocket, (const char*)(xptClient->sendBuffer->buffer), xptClient->sendBuffer->parserIndex, 0);
}

/*
 * Processes a fully received packet
 */
bool xptClient_processPacket(xptClient_t* xptClient)
{
	// printf("Received packet with opcode %d and size %d\n", xptClient->opcode, xptClient->recvSize+4);
	if( xptClient->opcode == XPT_OPC_S_AUTH_ACK )
		return xptClient_processPacket_authResponse(xptClient);
	else if( xptClient->opcode == XPT_OPC_S_WORKDATA1 )
		return xptClient_processPacket_blockData1(xptClient);
	else if( xptClient->opcode == XPT_OPC_S_SHARE_ACK )
		return xptClient_processPacket_shareAck(xptClient);


	// unknown opcodes are accepted too, for later backward compatibility
	return true;
}

/*
 * Checks for new received packets and connection events (e.g. closed connection)
 */
bool xptClient_process(xptClient_t* xptClient)
{
	
	if( xptClient == NULL )
		return false;
	// are there shares to submit?
	EnterCriticalSection(&xptClient->cs_shareSubmit);
	if( xptClient->list_shareSubmitQueue->objectCount > 0 )
	{
		for(uint32 i=0; i<xptClient->list_shareSubmitQueue->objectCount; i++)
		{
			xptShareToSubmit_t* xptShareToSubmit = (xptShareToSubmit_t*)xptClient->list_shareSubmitQueue->objects[i];
			xptClient_sendShare(xptClient, xptShareToSubmit);
			free(xptShareToSubmit);
		}
		// clear list
		xptClient->list_shareSubmitQueue->objectCount = 0;
	}
	LeaveCriticalSection(&xptClient->cs_shareSubmit);
	// check for packets
	sint32 packetFullSize = 4; // the packet always has at least the size of the header
	if( xptClient->recvSize > 0 )
		packetFullSize += xptClient->recvSize;
	sint32 bytesToReceive = (sint32)(packetFullSize - xptClient->recvIndex);
	// packet buffer is always large enough at this point
	sint32 r = recv(xptClient->clientSocket, (char*)(xptClient->recvBuffer->buffer+xptClient->recvIndex), bytesToReceive, 0);
	if( r <= 0 )
	{
		// receive error, is it a real error or just because of non blocking sockets?
		if( WSAGetLastError() != WSAEWOULDBLOCK )
		{
			xptClient->disconnected = true;
			return false;
		}
		return true;
	}
	xptClient->recvIndex += r;
	// header just received?
	if( xptClient->recvIndex == packetFullSize && packetFullSize == 4 )
	{
		// process header
		uint32 headerVal = *(uint32*)xptClient->recvBuffer->buffer;
		uint32 opcode = (headerVal&0xFF);
		uint32 packetDataSize = (headerVal>>8)&0xFFFFFF;
		// validate header size
		if( packetDataSize >= (1024*1024*2-4) )
		{
			// packets larger than 4mb are not allowed
			printf("xptServer_receiveData(): Packet exceeds 2mb size limit\n");
			return false;
		}
		xptClient->recvSize = packetDataSize;
		xptClient->opcode = opcode;
		// enlarge packetBuffer if too small
		if( (xptClient->recvSize+4) > xptClient->recvBuffer->bufferLimit )
		{
			xptPacketbuffer_changeSizeLimit(xptClient->recvBuffer, (xptClient->recvSize+4));
		}
	}
	// have we received the full packet?
	if( xptClient->recvIndex >= (xptClient->recvSize+4) )
	{
		// process packet
		xptClient->recvBuffer->bufferSize = (xptClient->recvSize+4);
		if( xptClient_processPacket(xptClient) == false )
		{
			xptClient->recvIndex = 0;
			xptClient->recvSize = 0;
			xptClient->opcode = 0;
			// disconnect
			if( xptClient->clientSocket != 0 )
			{
				closesocket(xptClient->clientSocket);
				xptClient->clientSocket = 0;
			}
			xptClient->disconnected = true;
			return false;
		}
		xptClient->recvIndex = 0;
		xptClient->recvSize = 0;
		xptClient->opcode = 0;
	}
	// return
	return true;
}

/*
 * Returns true if the xptClient connection was disconnected from the server or should disconnect because login was invalid or awkward data received
 */
bool xptClient_isDisconnected(xptClient_t* xptClient, char** reason)
{
	if( reason )
		*reason = xptClient->disconnectReason;
	return xptClient->disconnected;
}

/*
 * Returns true if the worker login was successful
 */
bool xptClient_isAuthenticated(xptClient_t* xptClient)
{
	return (xptClient->clientState == XPT_CLIENT_STATE_LOGGED_IN);
}

void xptClient_foundShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit)
{
	EnterCriticalSection(&xptClient->cs_shareSubmit);
	simpleList_add(xptClient->list_shareSubmitQueue, xptShareToSubmit);
	LeaveCriticalSection(&xptClient->cs_shareSubmit);
}