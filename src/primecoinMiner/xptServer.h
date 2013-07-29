typedef struct _xptServer_t xptServer_t;

typedef struct  
{
	uint8* buffer;
	uint32 parserIndex;
	uint32 bufferLimit; // current maximal size of buffer
	uint32 bufferSize; // current effective size of buffer
}xptPacketbuffer_t;

typedef struct  
{
	uint8 merkleRoot[32];
	uint32 seed;
}xptWorkData_t;

typedef struct  
{
	uint32 height;
	uint32 version;
	uint32 nTime;
	uint32 nBits;
	uint32 nBitsShare;
	uint8 prevBlock[32];
}xptBlockWorkInfo_t;

typedef struct _xptServer_t 
{
	SOCKET acceptSocket;
	simpleList_t* list_connections;
	xptPacketbuffer_t* sendBuffer; // shared buffer for sending data
	// last known block height (for new block detection)
	uint32 coinTypeBlockHeight[32];
	// callbacks
	bool (*xptCallback_generateWork)(xptServer_t* xptServer, uint32 numOfWorkEntries, uint32 coinTypeIndex, xptBlockWorkInfo_t* xptBlockWorkInfo, xptWorkData_t* xptWorkData);
	void (*xptCallback_getBlockHeight)(xptServer_t* xptServer, uint32* coinTypeNum, uint32* blockHeightPerCoinType);
}xptServer_t;

typedef struct  
{
	xptServer_t* xptServer;
	SOCKET clientSocket;
	bool disconnected;
	// recv buffer
	xptPacketbuffer_t* packetbuffer;
	uint32 recvIndex;
	uint32 recvSize;
	// recv header info
	uint32 opcode;
	// authentication info
	uint8 clientState;
	char workerName[128];
	char workerPass[128];
	uint32 userId;
	uint32 coinTypeIndex;
	uint32 payloadNum;

	//uint32 size;
	//// http auth
	//bool useBasicHTTPAuth;
	//char httpAuthUsername[64];
	//char httpAuthPassword[64];
	//// auto-diconnect
	//uint32 connectionOpenedTimer;
}xptServerClient_t;

// client states
#define XPT_CLIENT_STATE_NEW		(0)
#define XPT_CLIENT_STATE_LOGGED_IN	(1)


// list of known opcodes

#define XPT_OPC_C_AUTH_REQ		1
#define XPT_OPC_S_AUTH_ACK		2
#define XPT_OPC_S_WORKDATA1		3
#define XPT_OPC_C_SUBMIT_SHARE	4
#define XPT_OPC_S_SHARE_ACK		5

// list of error codes

#define XPT_ERROR_NONE				(0)
#define XPT_ERROR_INVALID_LOGIN		(1)
#define XPT_ERROR_INVALID_WORKLOAD	(2)
#define XPT_ERROR_INVALID_COINTYPE	(3)

// xpt general
xptServer_t* xptServer_create(uint16 port);
void xptServer_startProcessing(xptServer_t* xptServer);

// private packet handlers
bool xptServer_processPacket_authRequest(xptServer_t* xptServer, xptServerClient_t* xptServerClient);

// public packet methods
bool xptServer_sendBlockData(xptServer_t* xptServer, xptServerClient_t* xptServerClient);

// packetbuffer
xptPacketbuffer_t* xptPacketbuffer_create(uint32 initialSize);
void xptPacketbuffer_free(xptPacketbuffer_t* pb);
void xptPacketbuffer_changeSizeLimit(xptPacketbuffer_t* pb, uint32 sizeLimit);


void xptPacketbuffer_beginReadPacket(xptPacketbuffer_t* pb);
uint32 xptPacketbuffer_getReadSize(xptPacketbuffer_t* pb);
float xptPacketbuffer_readFloat(xptPacketbuffer_t* pb, bool* error);
uint32 xptPacketbuffer_readU32(xptPacketbuffer_t* pb, bool* error);
uint16 xptPacketbuffer_readU16(xptPacketbuffer_t* pb, bool* error);
uint8 xptPacketbuffer_readU8(xptPacketbuffer_t* pb, bool* error);
void xptPacketbuffer_readData(xptPacketbuffer_t* pb, uint8* data, uint32 length, bool* error);

void xptPacketbuffer_beginWritePacket(xptPacketbuffer_t* pb, uint8 opcode);
void xptPacketbuffer_writeFloat(xptPacketbuffer_t* pb, bool* error, float v);
void xptPacketbuffer_writeU32(xptPacketbuffer_t* pb, bool* error, uint32 v);
void xptPacketbuffer_writeU16(xptPacketbuffer_t* pb, bool* error, uint16 v);
void xptPacketbuffer_writeU8(xptPacketbuffer_t* pb, bool* error, uint8 v);
void xptPacketbuffer_writeData(xptPacketbuffer_t* pb, uint8* data, uint32 length, bool* error);
void xptPacketbuffer_finalizeWritePacket(xptPacketbuffer_t* pb);

void xptPacketbuffer_writeString(xptPacketbuffer_t* pb, char* stringData, uint32 maxStringLength, bool* error);
void xptPacketbuffer_readString(xptPacketbuffer_t* pb, char* stringData, uint32 maxStringLength, bool* error);