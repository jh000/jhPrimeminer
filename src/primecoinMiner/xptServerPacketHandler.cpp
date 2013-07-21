#include"global.h"

/*
 * Sends the response for an auth packet
 */
bool xptServer_sendAuthResponse(xptServer_t* xptServer, xptServerClient_t* xptServerClient, uint32 authErrorCode, char* rejectReason)
{
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptServer->sendBuffer, XPT_OPC_S_AUTH_ACK);
	xptPacketbuffer_writeU32(xptServer->sendBuffer, &sendError, authErrorCode);
	// write reject reason string (or motd in case of no error)
	sint32 rejectReasonLength = fStrLen(rejectReason);
	xptPacketbuffer_writeU16(xptServer->sendBuffer, &sendError, (uint16)rejectReasonLength);
	xptPacketbuffer_writeData(xptServer->sendBuffer, (uint8*)rejectReason, (uint32)rejectReasonLength, &sendError);
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptServer->sendBuffer);
	// send to client
	send(xptServerClient->clientSocket, (const char*)(xptServer->sendBuffer->buffer), xptServer->sendBuffer->parserIndex, 0);
	return true;
}

/*
 * Generates the block data and sends it to the client
 */
bool xptServer_sendBlockData(xptServer_t* xptServer, xptServerClient_t* xptServerClient)
{
	// we need several callbacks to the main work manager:
	if( xptServerClient->payloadNum < 1 || xptServerClient->payloadNum > 128 )
	{
		printf("xptServer_sendBlockData(): payloadNum out of range for worker %s\n", xptServerClient->workerName);
		return false;
	}
	// generate work
	xptBlockWorkInfo_t blockWorkInfo;
	xptWorkData_t workData[128];
	if( xptServer->xptCallback_generateWork(xptServer, xptServerClient->payloadNum, xptServerClient->coinTypeIndex, &blockWorkInfo, workData) == false )
	{
		printf("xptServer_sendBlockData(): Unable to generate work data for worker %s\n", xptServerClient->workerName);
		return false;
	}
	// build the packet
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptServer->sendBuffer, XPT_OPC_S_WORKDATA1);
	// add general block info
	xptPacketbuffer_writeU32(xptServer->sendBuffer, &sendError, blockWorkInfo.height);				// block height
	xptPacketbuffer_writeU32(xptServer->sendBuffer, &sendError, blockWorkInfo.nBits);				// nBits
	xptPacketbuffer_writeU32(xptServer->sendBuffer, &sendError, blockWorkInfo.nBitsShare);			// nBitsRecommended / nBitsShare
	xptPacketbuffer_writeU32(xptServer->sendBuffer, &sendError, blockWorkInfo.nTime);				// nTimestamp
	xptPacketbuffer_writeData(xptServer->sendBuffer, blockWorkInfo.prevBlock, 32, &sendError);		// prevBlockHash
	xptPacketbuffer_writeU32(xptServer->sendBuffer, &sendError, xptServerClient->payloadNum);		// payload num
	for(uint32 i=0; i<xptServerClient->payloadNum; i++)
	{
		// add merkle root for each work data entry
		xptPacketbuffer_writeData(xptServer->sendBuffer, workData[i].merkleRoot, 32, &sendError);
	}
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptServer->sendBuffer);
	// send to client
	send(xptServerClient->clientSocket, (const char*)(xptServer->sendBuffer->buffer), xptServer->sendBuffer->parserIndex, 0);
	return true;
}

/*
 * Called when an authentication request packet is received
 * This packet must arrive before any other packet
 */
bool xptServer_processPacket_authRequest(xptServer_t* xptServer, xptServerClient_t* xptServerClient)
{
	//if( xptServerClient->clientState != XPT_CLIENT_STATE_NEW )
	//	return false; // client already logged in or has other invalid state
	//xptPacketbuffer_t* cpb = xptServerClient->packetbuffer;
	//// read data from the packet
	//xptPacketbuffer_beginReadPacket(cpb);

	//uint32 	version;				//version of the x.pushthrough protocol used
	//uint32	usernameLength;			// range 1-128	
	//char	username[128+4];		// workername
	//uint32	passwordLength;			// range 1-128	
	//char	password[128+4];		// workername
	//uint32	payloadNum;				// number of different merkleRoots the server will generate for each block data request. Valid range: 1-128
	//// start parsing
	//bool readError = false;
	//// read version field
	//version = xptPacketbuffer_readU32(cpb, &readError);
	//if( readError )
	//	return false;
	//// read username length field
	//usernameLength = xptPacketbuffer_readU8(cpb, &readError);
	//if( readError )
	//	return false;
	//if( usernameLength < 1 || usernameLength > 128 )
	//	return false;
	//// read username
	//memset(username, 0x00, sizeof(username));
	//xptPacketbuffer_readData(cpb, (uint8*)username, usernameLength, &readError);
	//username[128] = '\0';
	//if( readError )
	//	return false;
	//// read password length field
	//passwordLength = xptPacketbuffer_readU8(cpb, &readError);
	//if( readError )
	//	return false;
	//if( passwordLength < 1 || passwordLength > 128 )
	//	return false;
	//// read password
	//memset(password, 0x00, sizeof(password));
	//xptPacketbuffer_readData(cpb, (uint8*)password, passwordLength, &readError);
	//password[128] = '\0';
	//if( readError )
	//	return false;
	//// read workloadPayRequest
	//payloadNum = xptPacketbuffer_readU32(cpb, &readError);
	//if( readError )
	//	return false;
	//// prepare reject stuff
	//uint8 rejectReasonCode = XPT_ERROR_NONE; // 0 -> no error
	//char* rejectReasonText = NULL;
	//// everything read successfully from the packet, validate worker
	//// get worker login
	//dbWorkerLogin_t* dbWorkerLogin = dbUtil_getWorkerLogin(username, password);
	//if( dbWorkerLogin == NULL )
	//{
	//	rejectReasonCode = XPT_ERROR_INVALID_LOGIN;
	//	rejectReasonText = "Invalid worker username or password";
	//	// send rejection response
	//	xptServer_sendAuthResponse(xptServer, xptServerClient, rejectReasonCode, rejectReasonText);
	//	// leave and disconnect client
	//	return false;
	//}
	//// worker login found - validate remaining data
	//if( payloadNum < 1 || payloadNum > 128 )
	//{
	//	rejectReasonCode = XPT_ERROR_INVALID_WORKLOAD;
	//	rejectReasonText = "Workload is out of range. 1-128 allowed.";
	//	// send rejection response
	//	xptServer_sendAuthResponse(xptServer, xptServerClient, rejectReasonCode, rejectReasonText);
	//	// leave and disconnect client
	//	return false;
	//}
	//uint16 coinTypeIndex = dbWorkerLogin->coinType-1;
	//// currently only Primecoin supports XPT
	//if( yPoolWorkMgr_getAlgorithm(coinTypeIndex) != ALGORITHM_PRIME )
	//{
	//	rejectReasonCode = XPT_ERROR_INVALID_COINTYPE;
	//	rejectReasonText = "Only primecoin workers are supported by x.pushthrough mode for now";
	//	// send rejection response
	//	xptServer_sendAuthResponse(xptServer, xptServerClient, rejectReasonCode, rejectReasonText);
	//	// leave and disconnect client
	//	return false;
	//}
	//// everything good so far, mark client as logged in and proceed
	//fStrCpy(xptServerClient->workerName, dbWorkerLogin->workername, 127);
	//fStrCpy(xptServerClient->workerPass, dbWorkerLogin->password, 127);
	//xptServerClient->userId = dbWorkerLogin->userId;
	////xptServerClient->workerId = dbWorkerLogin->
	//xptServerClient->coinTypeIndex = dbWorkerLogin->coinType-1;
	//xptServerClient->clientState = XPT_CLIENT_STATE_LOGGED_IN;
	//xptServerClient->payloadNum = payloadNum;
	//// send success response
	//char* motd = "";
	//xptServer_sendAuthResponse(xptServer, xptServerClient, XPT_ERROR_NONE, motd);
	//// immediately send first block of data
	//xptServer_sendBlockData(xptServer, xptServerClient);
	return true;
}