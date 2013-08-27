typedef struct  
{
	uint8 merkleRoot[32];
	uint8 prevBlockHash[32];
	uint32 version;
	uint32 nonce;
	uint32 nTime;
	uint32 nBits;
	// primecoin specific
	uint32 sieveSize;
	uint32 sieveCandidate; // index of sieveCandidate for this share
	uint8 fixedMultiplierSize;
	uint8 fixedMultiplier[201];
	uint8 chainMultiplierSize;
	uint8 chainMultiplier[201];
}xptShareToSubmit_t;

typedef struct  
{
	SOCKET clientSocket;
	xptPacketbuffer_t* sendBuffer; // buffer for sending data
	xptPacketbuffer_t* recvBuffer; // buffer for receiving data
	// worker info
	char username[128];
	char password[128];
	uint32 payloadNum;
	uint32 clientState;
	// recv info
	uint32 recvSize;
	uint32 recvIndex;
	uint32 opcode;
	// disconnect info
	bool disconnected;
	char* disconnectReason;
	// work data (queue)
	CRITICAL_SECTION cs_workAccess;
	xptBlockWorkInfo_t blockWorkInfo[400]; // queue for 400 work jobs, maybe replace with dynamic struct later
	uint32 blockWorkSize;
	float earnedShareValue; // this value is sent by the server with each new block that is sent
	// shares to submit
	CRITICAL_SECTION cs_shareSubmit;
	simpleList_t* list_shareSubmitQueue;
}xptClient_t;

xptClient_t* xptClient_connect(jsonRequestTarget_t* target, uint32 payloadNum);
void xptClient_free(xptClient_t* xptClient);

bool xptClient_process(xptClient_t* xptClient); // needs to be called in a loop
bool xptClient_isDisconnected(xptClient_t* xptClient, char** reason);
bool xptClient_isAuthenticated(xptClient_t* xptClient);

void xptClient_foundShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit);

// never send this directly
void xptClient_sendWorkerLogin(xptClient_t* xptClient);

// packet handlers
bool xptClient_processPacket_authResponse(xptClient_t* xptClient);
bool xptClient_processPacket_blockData1(xptClient_t* xptClient);
bool xptClient_processPacket_shareAck(xptClient_t* xptClient);
bool xptClient_processPacket_message(xptClient_t* xptClient);

// miner version string (needs to be defined somewhere in the project, max 45 characters)
extern char* minerVersionString;