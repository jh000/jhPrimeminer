#include"global.h"

SOCKET jsonRpc_openConnection(char *ip, int Port)
{
	SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(Port);
	addr.sin_addr.s_addr=inet_addr(ip);
	int result = connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN));
	if( result )
	{
		return 0;
	}
	return s;
}

/*
 * A full request was received from a client a can now be processed
 */
void jsonRpc_processRequest(jsonRpcServer_t* jrs, jsonRpcClient_t* client)
{
	char* requestData = (char*)(client->recvBuffer+client->recvDataHeaderEnd);
	sint32 requestLength = (sint32)(client->recvDataSizeFull - client->recvDataHeaderEnd);
	// parse data
	//jsonParser_parse(requestData, requestLength);
	jsonObject_t* jsonObject = jsonParser_parse((uint8*)requestData, requestLength);
	// get method
	jsonObject_t* jsonMethodName = jsonObject_getParameter(jsonObject, "method");
	jsonObject_t* jsonParameter = jsonObject_getParameter(jsonObject, "params");

	uint32 methodNameLength = 0;
	uint8* methodNameString = jsonObject_getStringData(jsonMethodName, &methodNameLength);
	if( methodNameString )
	{
		if( methodNameLength == 7 && memcmp(methodNameString, "getwork", 7) == 0 )
		{
			// ...
		}
		else
		{
			printf("JSON-RPC: Unknown method to call - ");
			for(uint32 i=0; i<methodNameLength; i++)
			{
				printf("%c", methodNameString[i]);
			}
			printf("\n");
		}
	}
	else
	{
		// invalid json data
		// kick the client
		closesocket(client->clientSocket);
		client->clientSocket = 0;
		client->disconnected = true;
	}
	jsonObject_freeObject(jsonObject);
	//// close connection (no keep alive)
	//closesocket(client->clientSocket);
	//client->clientSocket = 0;
	//client->disconnected = true;
	// reset pointers and indices in case the client sends another request (keep alive)
	client->recvIndex = 0;
	client->recvDataHeaderEnd = 0;
	client->recvDataSizeFull = 0;
}

/*
 * Creates a new instance of jsonRPC
 * port is the port identifier that is used for incoming connections
 * Returns NULL if the socket could not be created (most likely due to port being blocked)
 */
jsonRpcServer_t* jsonRpc_createServer(uint16 port)
{
	jsonRpcServer_t* jr = (jsonRpcServer_t*)malloc(sizeof(jsonRpcServer_t));
	RtlZeroMemory(jr, sizeof(jsonRpcServer_t));
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
		free(jr);
		return NULL;
	}
	listen(s, 64);
	jr->acceptSocket = s;
	// init misc
	jr->list_connections = simpleList_create(32);
	// return created server instance
	return jr;
}

/*
 * Called when the server detects a new incoming connection
 */
void jsonRpcServer_newClient(jsonRpcServer_t* jrs, SOCKET s)
{
	// alloc client struct
	jsonRpcClient_t* client = (jsonRpcClient_t*)malloc(sizeof(jsonRpcClient_t));
	RtlZeroMemory(client, sizeof(jsonRpcClient_t));
	// init client
	client->jsonRpcServer = jrs;
	client->clientSocket = s;
	// set socket as non-blocking
	unsigned int nonblocking=1;
	unsigned int cbRet;
	WSAIoctl(s, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);
	// init recv buffer
	client->recvIndex = 0;
	client->recvSize = JSON_INITIAL_RECV_BUFFER_SIZE;
	client->recvBuffer = (uint8*)malloc(JSON_INITIAL_RECV_BUFFER_SIZE);
	// add to client list
	simpleList_add(jrs->list_connections, client);
}

/*
 * Called when the server knows there is data to be received for a specific client
 * This method is also called when the client closes the connection
 */
bool jsonRpcServer_receiveData(jsonRpcServer_t* jrs, jsonRpcClient_t* client)
{
	uint32 remainingRecvSize = client->recvSize - client->recvIndex;
	if( remainingRecvSize == 0 )
	{
		// not enough data storage left
		// try to enlarge buffer if allowed
		if( client->recvSize < JSON_MAX_RECV_BUFFER_SIZE )
		{
			client->recvSize = min(client->recvSize*2, JSON_MAX_RECV_BUFFER_SIZE); // double buffer size
			client->recvBuffer = (uint8*)realloc(client->recvBuffer, client->recvSize);
			// recalculate remaining buffer storage
			remainingRecvSize = client->recvSize - client->recvIndex;
		}
		else
		{
			// evil client sends too much data
			// disconnect
			printf("JSON-RPC warning: Client tried to send more than 4MB of data\n");
			closesocket(client->clientSocket);
			client->clientSocket = 0;
			return false;
		}
	}
	// recv
	sint32 r = recv(client->clientSocket, (char*)(client->recvBuffer+client->recvIndex), remainingRecvSize, 0);
	if( r <= 0 )
	{
		closesocket(client->clientSocket);
		client->clientSocket = 0;
		return false;
	}
	else
	{
		client->recvIndex += r;
		// process data (if no header end found yet)
		if( client->recvDataHeaderEnd == 0 )
		{
			// did we receive the end of the header already?
			sint32 scanStart = (sint32)client->recvIndex - (sint32)r;
			scanStart = max(scanStart-8, 0);
			sint32 scanEnd = (sint32)(client->recvIndex);
			scanEnd = max(scanEnd-4, 0);
			for(sint32 s=scanStart; s<=scanEnd; s++)
			{
				// is header end?
				if( client->recvBuffer[s+0] == '\r' &&
					client->recvBuffer[s+1] == '\n' &&
					client->recvBuffer[s+2] == '\r' && 
					client->recvBuffer[s+3] == '\n' )
				{
					if( s == 0 )
					{
						printf("JSON-RPC warning: Client sent headerless HTTP request\n");
						closesocket(client->clientSocket);
						return false;
					}
					// reset some header related values
					client->useBasicHTTPAuth = false;
					// iterate header for content length field
					// when parsing the header we can assume two things
					// *) there will be always 4 bytes after endOfHeaderPtr
					// *) the 4 bytes at the end will always be \r\n\r\n
					char* endOfHeaderPtr = (char*)(client->recvBuffer+s);
					char* parsePtr = (char*)client->recvBuffer;
					sint32 contentLength = -1;
					while( parsePtr < endOfHeaderPtr )
					{
						// is content-length parameter?
						if( (endOfHeaderPtr-parsePtr) >= 15 && memcmp(parsePtr, "Content-Length:", 15) == 0 )
						{
							// parameter found
							// read value and end header parsing (for now)
							if( parsePtr[15] == ' ' ) // is there a space between the number and the colon?
								contentLength = atoi(parsePtr+16);
							else
								contentLength = atoi(parsePtr+15);
							break;
						}
						// is http authorization parameter?
						// 
						if( (endOfHeaderPtr-parsePtr) >= 21 && memcmp(parsePtr, "Authorization: Basic ", 21) == 0 )
						{
							// auth parameter found
							char* base64EncodedLogin = (char*)(parsePtr+21);
							uint8 httpLoginRaw[256];
							sint32 decodedLen = 0;
							base64_decode((const unsigned char*)base64EncodedLogin, strstr(base64EncodedLogin, "\r\n")-base64EncodedLogin, httpLoginRaw, &decodedLen);
							// make sure the httpLoginRaw string is null-terminated...
							httpLoginRaw[255] = '\0';
							// split into username and password
							char* offsetSplit = strstr((char*)httpLoginRaw, ":");
							if( offsetSplit )
							{
								char* username = (char*)httpLoginRaw;
								char* password = offsetSplit+1;
								*offsetSplit = '\0';
								fStrCpy(client->httpAuthUsername, username, 63);
								fStrCpy(client->httpAuthPassword, password, 63);
								client->useBasicHTTPAuth = true;
							}
							else
							{
								printf("JSON-RPC: Unable to parse HTTP authorization parameter\n");
							}
						}
						// continue to next line
						while( parsePtr < endOfHeaderPtr )
						{
							parsePtr++;
							if( parsePtr[0] == '\r' && parsePtr[1] == '\n' )
							{
								// next line found
								parsePtr += 2;
								break;
							}
						}
					}
					if( contentLength <= 0 )
					{
						printf("JSON-RPC warning: Content-Length header field not present\n");
						closesocket(client->clientSocket);
						return false;
					}
					// mark header and packet size, then finish processing header
					client->recvDataHeaderEnd = s+4;
					client->recvDataSizeFull = client->recvDataHeaderEnd + contentLength;
				}
			}
		}
		// process data (if header found and full packet received)
		if( client->recvDataHeaderEnd != 0 && client->recvIndex >= client->recvDataSizeFull )
		{
			// process request
			jsonRpc_processRequest(jrs, client);
			// wipe processed packet and shift remaining data back
			client->recvDataHeaderEnd = 0;
			uint32 pRecvIndex = client->recvIndex;
			client->recvIndex -= client->recvDataSizeFull;
			client->recvDataSizeFull = 0;
			if( client->recvIndex > 0 )
				__debugbreak(); // todo -> The client tried to send us more than one single request?
		}
	}
	return true;
}

/*
 * Frees all memory used by a client
 * Does not close connection
 */
void jsonRpc_deleteClient(jsonRpcClient_t* client)
{
	free(client->recvBuffer);
	free(client);
}

/*
 * Begins the jsonRPC server endless loop
 * Never quits unless jsonRpc_stop() is called (todo)
 * Can be run in a separate thread
 */
int jsonRpc_run(jsonRpcServer_t* jrs)
{
	FD_SET fd;
	timeval sTimeout;
	sTimeout.tv_sec = 1;
	sTimeout.tv_usec = 0;
	while( true )
	{
		FD_ZERO(&fd);
		// add server accept socket
		FD_SET(jrs->acceptSocket, &fd);
		// add all connected sockets
		for(uint32 i=0; i<jrs->list_connections->objectCount; i++)
		{
			jsonRpcClient_t* client = (jsonRpcClient_t*)simpleList_get(jrs->list_connections, i);
			FD_SET(client->clientSocket, &fd);
		}
		// check for socket events
		sint32 r = select(0, &fd, 0, 0, &sTimeout); // wait one second
		if( r )
		{
			// check for new connections
			if( FD_ISSET(jrs->acceptSocket, &fd) )
			{
				jsonRpcServer_newClient(jrs, accept(jrs->acceptSocket, 0, 0));
			}
			// check for client data received
			for(uint32 i=0; i<jrs->list_connections->objectCount; i++)
			{
				jsonRpcClient_t* client = (jsonRpcClient_t*)simpleList_get(jrs->list_connections, i);
				if( FD_ISSET(client->clientSocket, &fd) )
				{
					if( jsonRpcServer_receiveData(jrs, client) == false )
					{
						// something went wrong inside of _receiveData()
						// make sure the client gets disconnected
						client->disconnected = true;
					}
				}
				if( client->disconnected )
				{
					// delete client
					jsonRpc_deleteClient(client);
					// remove from list
					jrs->list_connections->objects[i] = jrs->list_connections->objects[jrs->list_connections->objectCount-1];
					jrs->list_connections->objectCount--;
				}
			}
		}
		else
			Sleep(1);
	}
	return 0;
}

void jsonRpc_sendResponse(jsonRpcServer_t* jrs, jsonRpcClient_t* client, jsonObject_t* jsonObjectReturnValue)
{
	// build json request data
	// example: {"method": "getwork", "params": [], "id":0}
	fStr_t* fStr_jsonRequestData = fStr_alloc(1024*64); // 64KB (this is also used as the recv buffer!)
	fStr_appendFormatted(fStr_jsonRequestData, "{ \"result\": ");
	// { "result": "Hallo JSON-RPC", "error": null, "id": 1}
	jsonBuilder_buildObjectString(fStr_jsonRequestData, jsonObjectReturnValue);
	fStr_append(fStr_jsonRequestData, ", \"error\": null, \"id\": 1");
	// prepare header
	fStr_buffer1kb_t fStrBuffer_header;
	fStr_t* fStr_headerData = fStr_alloc(&fStrBuffer_header, FSTR_FORMAT_UTF8);
	// header fields
	fStr_appendFormatted(fStr_headerData, "HTTP/1.1 200 OK\r\n");
	fStr_appendFormatted(fStr_headerData, "Server: ypoolbackend 0.1\r\n");
	fStr_appendFormatted(fStr_headerData, "Connection: keep-alive\r\n"); // no keep-alive
	fStr_appendFormatted(fStr_headerData, "Content-Type: application/json\r\n");
	fStr_appendFormatted(fStr_headerData, "Content-Length: %d\r\n", fStr_len(fStr_jsonRequestData));
	fStr_appendFormatted(fStr_headerData, "\r\n"); // empty line concludes the header
	// send header and data
	send(client->clientSocket, fStr_get(fStr_headerData), fStr_len(fStr_headerData), 0);
	send(client->clientSocket, fStr_get(fStr_jsonRequestData), fStr_len(fStr_jsonRequestData), 0);
	fStr_free(fStr_jsonRequestData);
}

/*
 * Allows to directly send raw json data instead of generating it on the fly
 */
void jsonRpc_sendResponseRaw(jsonRpcServer_t* jrs, jsonRpcClient_t* client, fStr_t* fStr_responseRawData, char* additionalHeaderData)
{
	// build json request data
	// example: {"method": "getwork", "params": [], "id":0}
	fStr_t* fStr_jsonRequestData = fStr_alloc(1024*64); // 64KB (this is also used as the recv buffer!)
	fStr_appendFormatted(fStr_jsonRequestData, "{ \"result\": ");
	// { "result": "Hallo JSON-RPC", "error": null, "id": 1}
	fStr_append(fStr_jsonRequestData, fStr_responseRawData);
	fStr_append(fStr_jsonRequestData, ", \"error\": null, \"id\": 1}");
	// prepare header
	fStr_buffer1kb_t fStrBuffer_header;
	fStr_t* fStr_headerData = fStr_alloc(&fStrBuffer_header, FSTR_FORMAT_UTF8);
	// header fields
	fStr_appendFormatted(fStr_headerData, "HTTP/1.1 200 OK\r\n");
	fStr_appendFormatted(fStr_headerData, "Server: ypoolbackend 0.1\r\n");
	fStr_appendFormatted(fStr_headerData, "Connection: keep-alive\r\n"); // keep-alive
	fStr_appendFormatted(fStr_headerData, "Content-Type: application/json\r\n");
	fStr_appendFormatted(fStr_headerData, "Content-Length: %d\r\n", fStr_len(fStr_jsonRequestData));
	if( additionalHeaderData )
		fStr_appendFormatted(fStr_headerData, "%s", additionalHeaderData);
	fStr_appendFormatted(fStr_headerData, "\r\n"); // empty line concludes the header
	// send header and data
	send(client->clientSocket, fStr_get(fStr_headerData), fStr_len(fStr_headerData), 0);
	send(client->clientSocket, fStr_get(fStr_jsonRequestData), fStr_len(fStr_jsonRequestData), 0);
	fStr_free(fStr_jsonRequestData);
}


/*
 * Allows to directly send raw json data instead of generating it on the fly
 */
void jsonRpc_sendFailedToAuthorize(jsonRpcServer_t* jrs, jsonRpcClient_t* client)
{
	// build json request data
	// example: {"method": "getwork", "params": [], "id":0}
	fStr_t* fStr_jsonRequestData = fStr_alloc(1024*64); // 64KB (this is also used as the recv buffer!)
	fStr_append(fStr_jsonRequestData, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\""
		"\"http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd\">"
		"<HTML>"
		"<HEAD>"
		"<TITLE>Error</TITLE>"
		"<META HTTP-EQUIV='Content-Type' CONTENT='text/html; charset=ISO-8859-1'>"
		"</HEAD>"
		"<BODY><H1>401 Unauthorized.</H1></BODY>"
		"</HTML>"
		);
	// prepare header
	fStr_buffer1kb_t fStrBuffer_header;
	fStr_t* fStr_headerData = fStr_alloc(&fStrBuffer_header, FSTR_FORMAT_UTF8);
	// header fields
	fStr_appendFormatted(fStr_headerData, "HTTP/1.0 401 Authorization Required\r\n");
	fStr_appendFormatted(fStr_headerData, "Server: ypoolbackend 0.1\r\n");
	fStr_appendFormatted(fStr_headerData, "WWW-Authenticate: Basic realm=\"jsonrpc\"\r\n");
	fStr_appendFormatted(fStr_headerData, "Connection: keep-alive\r\n"); // keep-alive
	fStr_appendFormatted(fStr_headerData, "Content-Type: text/html\r\n");
	fStr_appendFormatted(fStr_headerData, "Content-Length: %d\r\n", fStr_len(fStr_jsonRequestData));
	fStr_appendFormatted(fStr_headerData, "\r\n"); // empty line concludes the header
	// send header and data
	send(client->clientSocket, fStr_get(fStr_headerData), fStr_len(fStr_headerData), 0);
	send(client->clientSocket, fStr_get(fStr_jsonRequestData), fStr_len(fStr_jsonRequestData), 0);
	fStr_free(fStr_jsonRequestData);
}
