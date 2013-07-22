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
	xptClient->workDataValid = false;
	// add general block info
	xptClient->blockWorkInfo.version = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);			// version
	xptClient->blockWorkInfo.height = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);			// block height
	xptClient->blockWorkInfo.nBits = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);			// nBits
	xptClient->blockWorkInfo.nBitsShare = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);		// nBitsRecommended / nBitsShare
	xptClient->blockWorkInfo.nTime = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);			// nTimestamp
	xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->blockWorkInfo.prevBlock, 32, &recvError);	// prevBlockHash
	uint32 payloadNum = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);							// payload num
	if( recvError )
	{
		printf("xptClient_processPacket_blockData1(): Parse error\n");
		return false;
	}
	if( xptClient->payloadNum != payloadNum )
	{
		printf("xptClient_processPacket_blockData1(): Invalid payloadNum\n");
		return false;
	}
	for(uint32 i=0; i<payloadNum; i++)
	{
		// read merkle root for each work data entry
		xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->workData[i].merkleRoot, 32, &recvError);
	}
	if( recvError )
	{
		printf("xptClient_processPacket_blockData1(): Parse error 2\n");
		return false;
	}
	xptClient->workDataValid = true;
	xptClient->workDataCounter++;
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
	if( readError )
		return false;
	if( shareErrorCode == 0 )
	{
		total_shares++;
		valid_shares++;
		time_t now = time(0);
		char* dt = ctime(&now);
		printf("Valid share found!");
		printf("[ %d / %d ] %s",valid_shares, total_shares,dt);
	}
	else
	{
		// error logging in -> disconnect
		total_shares++;
		printf("Invalid share\n");
		if( rejectReason[0] != '\0' )
			printf("Reason: %s\n", rejectReason);
	}
	return true;
}