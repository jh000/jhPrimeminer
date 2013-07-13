extern uint8* json_emptyString;

typedef struct  
{
	uint8 type;
}jsonObject_t; // the base object struct, should never be allocated directly

typedef struct  
{
	uint8* stringNameData;
	uint32 stringNameLength;
	jsonObject_t* jsonObjectValue;
}jsonObjectRawObjectParameter_t;

typedef struct  
{
	jsonObject_t baseObject;
	customBuffer_t* list_paramPairs; // array of jsonObjectRawObjectParameter_t
}jsonObjectRawObject_t;

typedef struct  
{
	jsonObject_t baseObject;
	uint8* stringData;
	uint32 stringLength;
}jsonObjectString_t;

typedef struct  
{
	jsonObject_t baseObject;
	sint64 number;
	uint64 divider; // divide number by this factor to get the real result (usually divider will be 1)
}jsonObjectNumber_t;

typedef struct  
{
	jsonObject_t baseObject;
	bool isTrue;
}jsonObjectBool_t;

typedef struct  
{
	jsonObject_t baseObject;
	simpleList_t* list_values; // array of jsonObject_t ptrs
}jsonObjectArray_t;

#define JSON_TYPE_OBJECT	(1)
#define JSON_TYPE_STRING	(2)
#define JSON_TYPE_ARRAY		(3)
#define JSON_TYPE_NUMBER	(4)
#define JSON_TYPE_NULL		(5)
#define JSON_TYPE_BOOL		(6)

typedef struct  
{
	uint8* dataBuffer;
	uint32 dataLength;
	uint8* dataCurrentPtr;
	uint8* dataEnd;
}jsonParser_t;

typedef struct  
{
	char* ip;
	uint16 port;
	char* authUser;
	char* authPass;
}jsonRequestTarget_t;

#define JSON_ERROR_NONE					(0)		// no error (success)
#define JSON_ERROR_HOST_NOT_FOUND		(1)		// unable to connect to ip/port
#define JSON_ERROR_UNABLE_TO_PARSE		(2)		// error parsing the returned JSON data
#define JSON_ERROR_NO_RESULT			(3)		// missing result parameter


jsonObject_t* jsonParser_parse(uint8* stream, uint32 dataLength);

typedef struct _jsonRpcServer_t jsonRpcServer_t;

#define JSON_INITIAL_RECV_BUFFER_SIZE	(1024*4) // 4KB
#define JSON_MAX_RECV_BUFFER_SIZE	(1024*1024*4) // 4MB

typedef struct  
{
	jsonRpcServer_t* jsonRpcServer;
	SOCKET clientSocket;
	bool disconnected;
	// recv buffer
	uint32 recvIndex;
	uint32 recvSize;
	uint8* recvBuffer;
	// recv header info
	uint32 recvDataSizeFull;
	uint32 recvDataHeaderEnd;
	// http auth
	bool useBasicHTTPAuth;
	char httpAuthUsername[64];
	char httpAuthPassword[64];
}jsonRpcClient_t;

typedef struct _jsonRpcServer_t 
{
	SOCKET acceptSocket;
	simpleList_t* list_connections;
}jsonRpcServer_t;

// server
jsonRpcServer_t* jsonRpc_createServer(uint16 port);
int jsonRpc_run(jsonRpcServer_t* jrs);
void jsonRpc_sendResponse(jsonRpcServer_t* jrs, jsonRpcClient_t* client, jsonObject_t* jsonObjectReturnValue);
void jsonRpc_sendResponseRaw(jsonRpcServer_t* jrs, jsonRpcClient_t* client, fStr_t* fStr_responseRawData, char* additionalHeaderData);
void jsonRpc_sendFailedToAuthorize(jsonRpcServer_t* jrs, jsonRpcClient_t* client);

// object
jsonObject_t* jsonObject_getParameter(jsonObject_t* jsonObject, char* name);
uint32 jsonObject_getType(jsonObject_t* jsonObject);
uint8* jsonObject_getStringData(jsonObject_t* jsonObject, uint32* length);
uint32 jsonObject_getArraySize(jsonObject_t* jsonObject);
jsonObject_t* jsonObject_getArrayElement(jsonObject_t* jsonObject, uint32 index);
bool jsonObject_isTrue(jsonObject_t* jsonObject);
double jsonObject_getNumberValueAsDouble(jsonObject_t* jsonObject);
sint32 jsonObject_getNumberValueAsS32(jsonObject_t* jsonObject);

// object deallocation
void jsonObject_freeStringData(uint8* stringBuffer);
void jsonObject_freeObject(jsonObject_t* jsonObject);

// builder
void jsonBuilder_buildObjectString(fStr_t* fStr_output, jsonObject_t* jsonObject);

// client
jsonObject_t* jsonClient_request(jsonRequestTarget_t* server, char* methodName, fStr_t* fStr_parameterData, sint32* errorCode);

// base64
int base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, char* output);
unsigned char * base64_decode(const unsigned char *src, size_t len, uint8* out, sint32 *out_len);