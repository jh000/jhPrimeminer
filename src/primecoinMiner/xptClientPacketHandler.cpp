#include"global.h"

/*
 * Called when a packet with the opcode XPT_OPC_S_AUTH_ACK is received
 */
bool xptClient_processPacket_authResponse(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read error code field
	uint32 authErrorCode = xptPacketbuffer_readU32(cpb, &readError);
	if( readError )
		return false;
	// read reject reason / motd
	char rejectReason[512];
	xptPacketbuffer_readString(cpb, rejectReason, 512, &readError);
	rejectReason[511] = '\0';
	if( readError )
		return false;
	if( authErrorCode == 0 )
	{
		xptClient->clientState = XPT_CLIENT_STATE_LOGGED_IN;
		printf("xpt: Logged in\n");
		if( rejectReason[0] != '\0' )
			printf("Message from server: %s\n", rejectReason);
	}
	else
	{
		// error logging in -> disconnect
		printf("xpt: Failed to log in\n");
		if( rejectReason[0] != '\0' )
			printf("Reason: %s\n", rejectReason);
		return false;
	}
	return true;
}

/*
 * Called when a packet with the opcode XPT_OPC_S_WORKDATA1 is received
 * This is the first version of xpt 'getwork'
 */
bool xptClient_processPacket_blockData1(xptClient_t* xptClient)
{
	// parse block data
	bool recvError = false;
	xptPacketbuffer_beginReadPacket(xptClient->recvBuffer);
	// add general block info
	EnterCriticalSection(&xptClient->cs_workAccess);
	float earnedShareValue = xptPacketbuffer_readFloat(xptClient->recvBuffer, &recvError);
	xptClient->earnedShareValue += earnedShareValue;
	uint32 numWorkBundle = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError); // how many workBundle blocks we have
	for(uint32 b=0; b<numWorkBundle; b++)
	{
		// general block info
		uint32 blockVersion = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 blockHeight = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 blockBits = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 blockBitsForShare = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 blockTimestamp = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 workBundleFlags = xptPacketbuffer_readU8(xptClient->recvBuffer, &recvError);
		uint8 prevBlockHash[32];
		xptPacketbuffer_readData(xptClient->recvBuffer, prevBlockHash, 32, &recvError);
		// server constraints
		uint32 fixedPrimorial = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 fixedHashFactor = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 sievesizeMin = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 sievesizeMax = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 primesToSieveMin = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 primesToSieveMax = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 sieveChainLength = xptPacketbuffer_readU8(xptClient->recvBuffer, &recvError);
		uint32 nonceMin = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 nonceMax = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		uint32 numPayload =  xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
		for(uint32 p=0; p<numPayload; p++)
		{
			xptBlockWorkInfo_t* blockData = xptClient->blockWorkInfo + xptClient->blockWorkSize;
			if( xptClient->blockWorkSize >= 400 )
				break;
			blockData->version = blockVersion;
			blockData->height = blockHeight;
			blockData->nBits = blockBits;
			blockData->nBitsShare = blockBitsForShare;
			blockData->nTime = blockTimestamp;
			memcpy(blockData->prevBlockHash, prevBlockHash, 32);
			blockData->flags = workBundleFlags;
			blockData->fixedPrimorial = fixedPrimorial;
			blockData->fixedHashFactor = fixedHashFactor;
			blockData->sievesizeMin = sievesizeMin;
			blockData->sievesizeMax = sievesizeMax;
			blockData->primesToSieveMin = primesToSieveMin;
			blockData->primesToSieveMax = primesToSieveMax;
			blockData->sieveChainLength = sieveChainLength;
			blockData->nonceMin = nonceMin;
			blockData->nonceMax = nonceMax;
			blockData->flags = workBundleFlags;
			xptPacketbuffer_readData(xptClient->recvBuffer, blockData->merkleRoot, 32, &recvError);
			xptClient->blockWorkSize++;
		}
	}
	LeaveCriticalSection(&xptClient->cs_workAccess);
	return true;
}

/*
 * Called when a packet with the opcode XPT_OPC_S_SHARE_ACK is received
 */
bool xptClient_processPacket_shareAck(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read error code field
	uint32 shareErrorCode = xptPacketbuffer_readU32(cpb, &readError);
	if( readError )
		return false;
	// read reject reason
	char rejectReason[512];
	xptPacketbuffer_readString(cpb, rejectReason, 512, &readError);
	rejectReason[511] = '\0';
	float shareValue = xptPacketbuffer_readFloat(cpb, &readError);
	if( readError )
		return false;
	if( shareErrorCode == 0 )
	{
		total_shares++;
		valid_shares++;
		time_t now = time(0);
		char* dt = ctime(&now);
		printf("Share accepted by server");
		printf(" [ %d / %d val: %.6f] %s", valid_shares, total_shares, shareValue, dt);
		primeStats.fShareValue += shareValue;
	}
	else
	{
		// share not accepted by server
		total_shares++;
		printf("Invalid share\n");
		if( rejectReason[0] != '\0' )
			printf("Reason: %s\n", rejectReason);
	}
	return true;
}


/*
 * Called when a packet with the opcode XPT_OPC_S_MESSAGE is received
 */
bool xptClient_processPacket_message(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read type field (not used yet)
	uint32 messageType = xptPacketbuffer_readU8(cpb, &readError);
	if( readError )
		return false;
	// read message text (up to 1024 bytes)
	char messageText[1024];
	xptPacketbuffer_readString(cpb, messageText, 1024, &readError);
	messageText[1023] = '\0';
	if( readError )
		return false;
	printf("Server message: %s\n", messageText);
	return true;
}
