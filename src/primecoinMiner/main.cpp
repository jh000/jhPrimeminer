#include"global.h"

#include<intrin.h>
#include<ctime>

primeStats_t primeStats = {0};
volatile int total_shares = 0;
volatile int valid_shares = 0;
unsigned int nMaxSieveSize;
char* dt;

bool error(const char *format, ...)
{
	puts(format);
	//__debugbreak();
	return false;
}

uint32 _swapEndianessU32(uint32 v)
{
	return ((v>>24)&0xFF)|((v>>8)&0xFF00)|((v<<8)&0xFF0000)|((v<<24)&0xFF000000);
}

uint32 _getHexDigitValue(uint8 c)
{
	if( c >= '0' && c <= '9' )
		return c-'0';
	else if( c >= 'a' && c <= 'f' )
		return c-'a'+10;
	else if( c >= 'A' && c <= 'F' )
		return c-'A'+10;
	return 0;
}

/*
 * Parses a hex string
 * Length should be a multiple of 2
 */
void yPoolWorkMgr_parseHexString(char* hexString, uint32 length, uint8* output)
{
	uint32 lengthBytes = length / 2;
	for(uint32 i=0; i<lengthBytes; i++)
	{
		// high digit
		uint32 d1 = _getHexDigitValue(hexString[i*2+0]);
		// low digit
		uint32 d2 = _getHexDigitValue(hexString[i*2+1]);
		// build byte
		output[i] = (uint8)((d1<<4)|(d2));	
	}
}

/*
 * Parses a hex string and converts it to LittleEndian (or just opposite endianness)
 * Length should be a multiple of 2
 */
void yPoolWorkMgr_parseHexStringLE(char* hexString, uint32 length, uint8* output)
{
	uint32 lengthBytes = length / 2;
	for(uint32 i=0; i<lengthBytes; i++)
	{
		// high digit
		uint32 d1 = _getHexDigitValue(hexString[i*2+0]);
		// low digit
		uint32 d2 = _getHexDigitValue(hexString[i*2+1]);
		// build byte
		output[lengthBytes-i-1] = (uint8)((d1<<4)|(d2));	
	}
}


void primecoinBlock_generateHeaderHash(primecoinBlock_t* primecoinBlock, uint8 hashOutput[32])
{
	uint8 blockHashDataInput[512];
	memcpy(blockHashDataInput, primecoinBlock, 80);
	sha256_context ctx;
	sha256_starts(&ctx);
	sha256_update(&ctx, (uint8*)blockHashDataInput, 80);
	sha256_finish(&ctx, hashOutput);
	sha256_starts(&ctx); // is this line needed?
	sha256_update(&ctx, hashOutput, 32);
	sha256_finish(&ctx, hashOutput);
}

void primecoinBlock_generateBlockHash(primecoinBlock_t* primecoinBlock, uint8 hashOutput[32])
{
	uint8 blockHashDataInput[512];
	memcpy(blockHashDataInput, primecoinBlock, 80);
	uint32 writeIndex = 80;
	sint32 lengthBN = 0;
	std::vector<unsigned char> bnSerializeData = primecoinBlock->bnPrimeChainMultiplier.getvch();
	lengthBN = bnSerializeData.size();
	*(uint8*)(blockHashDataInput+writeIndex) = (uint8)lengthBN;
	writeIndex += 1;
	memcpy(blockHashDataInput+writeIndex, &bnSerializeData[0], lengthBN);
	writeIndex += lengthBN;
	sha256_context ctx;
	sha256_starts(&ctx);
	sha256_update(&ctx, (uint8*)blockHashDataInput, writeIndex);
	sha256_finish(&ctx, hashOutput);
	sha256_starts(&ctx); // is this line needed?
	sha256_update(&ctx, hashOutput, 32);
	sha256_finish(&ctx, hashOutput);
}

typedef struct  
{
	bool dataIsValid;
	uint8 data[128];
	uint32 dataHash; // used to detect work data changes
	uint8 serverData[32]; // contains data from the server 
}workDataEntry_t;

typedef struct  
{
	CRITICAL_SECTION cs;
	uint8 protocolMode;
	// xpm
	workDataEntry_t workEntry[128]; // work data for each thread (up to 128)
	// x.pushthrough
	xptClient_t* xptClient;
}workData_t;

#define MINER_PROTOCOL_GETWORK		(1)
#define MINER_PROTOCOL_STRATUM		(2)
#define MINER_PROTOCOL_XPUSHTHROUGH	(3)

workData_t workData;

jsonRequestTarget_t jsonRequestTarget; // rpc login data


/*
 * Pushes the found block data to the server for giving us the $$$
 * Uses getwork to push the block
 * Returns true on success
 * Note that the primecoin data can be larger due to the multiplier at the end, so we use 256 bytes per default
 */
bool jhMiner_pushShare_primecoin(uint8 data[256], primecoinBlock_t* primecoinBlock)
{
	if( workData.protocolMode == MINER_PROTOCOL_GETWORK )
	{
		// prepare buffer to send
		fStr_buffer4kb_t fStrBuffer_parameter;
		fStr_t* fStr_parameter = fStr_alloc(&fStrBuffer_parameter, FSTR_FORMAT_UTF8);
		fStr_append(fStr_parameter, "[\""); // \"]
		fStr_addHexString(fStr_parameter, data, 256);
		fStr_appendFormatted(fStr_parameter, "\",\"");
		fStr_addHexString(fStr_parameter, (uint8*)&primecoinBlock->serverData, 32);
		fStr_append(fStr_parameter, "\"]");
		// send request
		sint32 rpcErrorCode = 0;
		jsonObject_t* jsonReturnValue = jsonClient_request(&jsonRequestTarget, "getwork", fStr_parameter, &rpcErrorCode);
		if( jsonReturnValue == NULL )
		{
			printf("PushWorkResult failed :(\n");
			return false;
		}
		else
		{
			// rpc call worked, sooooo.. is the server happy with the result?
			jsonObject_t* jsonReturnValueBool = jsonObject_getParameter(jsonReturnValue, "result");
			if( jsonObject_isTrue(jsonReturnValueBool) )
			{
				total_shares++;
				valid_shares++;
				time_t now = time(0);
				dt = ctime(&now);
				printf("Valid share found!");
				printf("[ %d / %d ] %s",valid_shares, total_shares,dt);
				jsonObject_freeObject(jsonReturnValue);
				return true;
			}
			else
			{
				total_shares++;
				// the server says no to this share :(
				printf("Server rejected share (BlockHeight: %d/%d nBits: 0x%08X)\n", primecoinBlock->serverData.blockHeight, jhMiner_getCurrentWorkBlockHeight(primecoinBlock->threadIndex), primecoinBlock->serverData.client_shareBits);
				jsonObject_freeObject(jsonReturnValue);
				return false;
			}
		}
		jsonObject_freeObject(jsonReturnValue);
		return false;
	}
	else if( workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH )
	{
		// printf("Queue share\n");
		xptShareToSubmit_t* xptShareToSubmit = (xptShareToSubmit_t*)malloc(sizeof(xptShareToSubmit_t));
		memset(xptShareToSubmit, 0x00, sizeof(xptShareToSubmit_t));
		memcpy(xptShareToSubmit->merkleRoot, primecoinBlock->merkleRoot, 32);
		memcpy(xptShareToSubmit->prevBlockHash, primecoinBlock->prevBlockHash, 32);
		xptShareToSubmit->version = primecoinBlock->version;
		xptShareToSubmit->nBits = primecoinBlock->nBits;
		xptShareToSubmit->nonce = primecoinBlock->nonce;
		xptShareToSubmit->nTime = primecoinBlock->timestamp;
		// set multiplier
		std::vector<unsigned char> bnSerializeData = primecoinBlock->bnPrimeChainMultiplier.getvch();
		sint32 lengthBN = bnSerializeData.size();
		memcpy(xptShareToSubmit->chainMultiplier, &bnSerializeData[0], lengthBN);
		xptShareToSubmit->chainMultiplierSize = lengthBN;
		// todo: Set stuff like sieve size
		if( workData.xptClient )
			xptClient_foundShare(workData.xptClient, xptShareToSubmit);
	}
}

void jhMiner_queryWork_primecoin()
{
	sint32 rpcErrorCode = 0;
	uint32 time1 = GetTickCount();
	jsonObject_t* jsonReturnValue = jsonClient_request(&jsonRequestTarget, "getwork", NULL, &rpcErrorCode);
	uint32 time2 = GetTickCount() - time1;
	// printf("request time: %dms\n", time2);
	if( jsonReturnValue == NULL )
	{
		printf("Getwork() failed with %serror code %d\n", (rpcErrorCode>1000)?"http ":"", rpcErrorCode>1000?rpcErrorCode-1000:rpcErrorCode);
		workData.workEntry[0].dataIsValid = false;
		return;
	}
	else
	{
		jsonObject_t* jsonResult = jsonObject_getParameter(jsonReturnValue, "result");
		jsonObject_t* jsonResult_data = jsonObject_getParameter(jsonResult, "data");
		//jsonObject_t* jsonResult_hash1 = jsonObject_getParameter(jsonResult, "hash1");
		jsonObject_t* jsonResult_target = jsonObject_getParameter(jsonResult, "target");
		jsonObject_t* jsonResult_serverData = jsonObject_getParameter(jsonResult, "serverData");
		//jsonObject_t* jsonResult_algorithm = jsonObject_getParameter(jsonResult, "algorithm");
		if( jsonResult_data == NULL )
		{
			printf("Error :(\n");
			workData.workEntry[0].dataIsValid = false;
			jsonObject_freeObject(jsonReturnValue);
			return;
		}
		// data
		uint32 stringData_length = 0;
		uint8* stringData_data = jsonObject_getStringData(jsonResult_data, &stringData_length);
		//printf("data: %.*s...\n", (sint32)min(48, stringData_length), stringData_data);

		EnterCriticalSection(&workData.cs);
		yPoolWorkMgr_parseHexString((char*)stringData_data, min(128*2, stringData_length), workData.workEntry[0].data);
		workData.workEntry[0].dataIsValid = true;
		// get server data
		uint32 stringServerData_length = 0;
		uint8* stringServerData_data = jsonObject_getStringData(jsonResult_serverData, &stringServerData_length);
		RtlZeroMemory(workData.workEntry[0].serverData, 32);
		if( jsonResult_serverData )
			yPoolWorkMgr_parseHexString((char*)stringServerData_data, min(128*2, 32*2), workData.workEntry[0].serverData);
		// generate work hash
		uint32 workDataHash = 0x5B7C8AF4;
		for(uint32 i=0; i<stringData_length/2; i++)
		{
			workDataHash = (workDataHash>>29)|(workDataHash<<3);
			workDataHash += (uint32)workData.workEntry[0].data[i];
		}
		workData.workEntry[0].dataHash = workDataHash;
		LeaveCriticalSection(&workData.cs);
		jsonObject_freeObject(jsonReturnValue);
	}
}

/*
 * Returns the block height of the most recently received workload
 */
uint32 jhMiner_getCurrentWorkBlockHeight(sint32 threadIndex)
{
	if( workData.protocolMode == MINER_PROTOCOL_GETWORK )
		return ((serverData_t*)workData.workEntry[0].serverData)->blockHeight;	
	else
		return ((serverData_t*)workData.workEntry[threadIndex].serverData)->blockHeight;
}

/*
 * Worker thread mainloop for getwork() mode
 */
int jhMiner_workerThread_getwork(int threadIndex)
{
	while( true )
	{
		uint8 localBlockData[128];
		// copy block data from global workData
		uint32 workDataHash = 0;
		uint8 serverData[32];
		while( workData.workEntry[0].dataIsValid == false ) Sleep(200);
		EnterCriticalSection(&workData.cs);
		memcpy(localBlockData, workData.workEntry[0].data, 128);
		//seed = workData.seed;
		memcpy(serverData, workData.workEntry[0].serverData, 32);
		LeaveCriticalSection(&workData.cs);
		// swap endianess
		for(uint32 i=0; i<128/4; i++)
		{
			*(uint32*)(localBlockData+i*4) = _swapEndianessU32(*(uint32*)(localBlockData+i*4));
		}
		// convert raw data into primecoin block
		primecoinBlock_t primecoinBlock = {0};
		memcpy(&primecoinBlock, localBlockData, 80);
		// we abuse the timestamp field to generate an unique hash for each worker thread...
		primecoinBlock.timestamp += threadIndex;
		primecoinBlock.threadIndex = threadIndex;
		primecoinBlock.xptMode = (workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH);
		// ypool uses a special encrypted serverData value to speedup identification of merkleroot and share data
		memcpy(&primecoinBlock.serverData, serverData, 32);
		// start mining
		BitcoinMiner(&primecoinBlock, threadIndex);
		primecoinBlock.bnPrimeChainMultiplier = 0;
	}
	return 0;
}

/*
 * Worker thread mainloop for xpt() mode
 */
int jhMiner_workerThread_xpt(int threadIndex)
{
	while( true )
	{
		uint8 localBlockData[128];
		// copy block data from global workData
		uint32 workDataHash = 0;
		uint8 serverData[32];
		while( workData.workEntry[threadIndex].dataIsValid == false ) Sleep(200);
		EnterCriticalSection(&workData.cs);
		memcpy(localBlockData, workData.workEntry[threadIndex].data, 128);
		memcpy(serverData, workData.workEntry[threadIndex].serverData, 32);
		workDataHash = workData.workEntry[threadIndex].dataHash;
		LeaveCriticalSection(&workData.cs);
		// convert raw data into primecoin block
		primecoinBlock_t primecoinBlock = {0};
		memcpy(&primecoinBlock, localBlockData, 80);
		// we abuse the timestamp field to generate an unique hash for each worker thread...
		primecoinBlock.timestamp += threadIndex;
		primecoinBlock.threadIndex = threadIndex;
		primecoinBlock.xptMode = (workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH);
		// ypool uses a special encrypted serverData value to speedup identification of merkleroot and share data
		memcpy(&primecoinBlock.serverData, serverData, 32);
		// start mining
		//uint32 time1 = GetTickCount();
		BitcoinMiner(&primecoinBlock, threadIndex);
		//printf("Mining stopped after %dms\n", GetTickCount()-time1);
		primecoinBlock.bnPrimeChainMultiplier = 0;
	}
	return 0;
}

typedef struct  
{
	char* workername;
	char* workerpass;
	char* host;
	sint32 port;
	sint32 numThreads;
	sint32 sieveSize;
}commandlineInput_t;

commandlineInput_t commandlineInput = {0};

void jhMiner_printHelp()
{
	puts("Usage: jhPrimeminer.exe [options]");
	puts("Options:");
	puts("   -o, -O                        The miner will connect to this url");
	puts("                                 You can specifiy an port after the url using -o url:port");
	puts("   -u                            The username (workername) used for login");
	puts("   -p                            The password used for login");
	puts("   -t                            The number of threads for mining (default 1)");
	puts("                                 For most efficient mining, set to number of cores");
	puts("   -s                            Set MaxSieveSize range from 200000 - 10000000");
	puts("                                 Default is 1000000.");
	puts("Example usage:");
	puts("   jhPrimeminer.exe -o http://poolurl.com:8332 -u workername.1 -p workerpass -t 4");
}

void jhMiner_parseCommandline(int argc, char **argv)
{
	sint32 cIdx = 1;
	while( cIdx < argc )
	{
		char* argument = argv[cIdx];
		cIdx++;
		if( memcmp(argument, "-o", 2)==0 || memcmp(argument, "-O", 2)==0 )
		{
			// -o
			if( cIdx >= argc )
			{
				printf("Missing URL after -o option\n");
				ExitProcess(0);
			}
			if( strstr(argv[cIdx], "http://") )
				commandlineInput.host = fStrDup(strstr(argv[cIdx], "http://")+7);
			else
				commandlineInput.host = fStrDup(argv[cIdx]);
			char* portStr = strstr(commandlineInput.host, ":");
			if( portStr )
			{
				*portStr = '\0';
				commandlineInput.port = atoi(portStr+1);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-u", 2)==0 )
		{
			// -u
			if( cIdx >= argc )
			{
				printf("Missing username/workername after -u option\n");
				ExitProcess(0);
			}
			commandlineInput.workername = fStrDup(argv[cIdx], 64);
			cIdx++;
		}
		else if( memcmp(argument, "-p", 2)==0 )
		{
			// -p
			if( cIdx >= argc )
			{
				printf("Missing password after -p option\n");
				ExitProcess(0);
			}
			commandlineInput.workerpass = fStrDup(argv[cIdx], 64);
			cIdx++;
		}
		else if( memcmp(argument, "-t", 2)==0 )
		{
			// -t
			if( cIdx >= argc )
			{
				printf("Missing thread number after -t option\n");
				ExitProcess(0);
			}
			commandlineInput.numThreads = atoi(argv[cIdx]);
			if( commandlineInput.numThreads < 1 || commandlineInput.numThreads > 128 )
			{
				printf("-t parameter out of range");
				ExitProcess(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-s", 2)==0 )
		{
			// -s
			if( cIdx >= argc )
			{
				printf("Missing number after -s option\n");
				ExitProcess(0);
			}
			commandlineInput.sieveSize = atoi(argv[cIdx]);
			if( commandlineInput.sieveSize < 200000 || commandlineInput.sieveSize > 10000000 )
			{
				printf("-s parameter out of range, must be between 200000 - 10000000");
				ExitProcess(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-help", 5)==0 || memcmp(argument, "--help", 6)==0 )
		{
			jhMiner_printHelp();
			ExitProcess(0);
		}
		else
		{
			printf("'%s' is an unknown option.\nType jhPrimeminer.exe --help for more info\n", argument); 
			ExitProcess(-1);
		}
	}
	if( argc <= 1 )
	{
		jhMiner_printHelp();
		ExitProcess(0);
	}
}

/*
 * Mainloop when using getwork() mode
 */
int jhMiner_main_getworkMode()
{
	// start threads
	for(sint32 threadIdx=0; threadIdx<commandlineInput.numThreads; threadIdx++)
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)jhMiner_workerThread_getwork, (LPVOID)threadIdx, 0, 0);
	// main thread, query work every 8 seconds
	sint32 loopCounter = 0;
	while( true )
	{
		// query new work
		jhMiner_queryWork_primecoin();
		// calculate stats every second tick
		if( loopCounter&1 )
		{
			double statsPassedTime = (double)(GetTickCount() - primeStats.primeLastUpdate);
			if( statsPassedTime < 1.0 )
				statsPassedTime = 1.0; // avoid division by zero
			double primesPerSecond = (double)primeStats.primeChainsFound / (statsPassedTime / 1000.0);
			primeStats.primeLastUpdate = GetTickCount();
			primeStats.primeChainsFound = 0;
			uint32 bestDifficulty = primeStats.bestPrimeChainDifficulty;
			primeStats.bestPrimeChainDifficulty = 0;
			double primeDifficulty = (double)bestDifficulty / (double)0x1000000;
			if( workData.workEntry[0].dataIsValid )
			{
				primeStats.bestPrimeChainDifficultySinceLaunch = max(primeStats.bestPrimeChainDifficultySinceLaunch, primeDifficulty);
				printf("primes/s: %d best difficulty: %f record: %f\n", (sint32)primesPerSecond, (float)primeDifficulty, (float)primeStats.bestPrimeChainDifficultySinceLaunch);
			}
		}		
		// wait and check some stats
		uint32 time_updateWork = GetTickCount();
		while( true )
		{
			uint32 passedTime = GetTickCount() - time_updateWork;
			if( passedTime >= 4000 )
				break;
			Sleep(200);
		}
		loopCounter++;
	}
	return 0;
}

/*
 * Mainloop when using xpt mode
 */
int jhMiner_main_xptMode()
{
	// start threads
	for(sint32 threadIdx=0; threadIdx<commandlineInput.numThreads; threadIdx++)
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)jhMiner_workerThread_xpt, (LPVOID)threadIdx, 0, 0);
	// main thread, don't query work, just wait and process
	sint32 loopCounter = 0;
	uint32 xptWorkIdentifier = 0xFFFFFFFF;
	while( true )
	{
		// calculate stats every second tick
		if( loopCounter&1 )
		{
			double statsPassedTime = (double)(GetTickCount() - primeStats.primeLastUpdate);
			if( statsPassedTime < 1.0 )
				statsPassedTime = 1.0; // avoid division by zero
			double primesPerSecond = (double)primeStats.primeChainsFound / (statsPassedTime / 1000.0);
			primeStats.primeLastUpdate = GetTickCount();
			primeStats.primeChainsFound = 0;
			uint32 bestDifficulty = primeStats.bestPrimeChainDifficulty;
			primeStats.bestPrimeChainDifficulty = 0;
			double primeDifficulty = (double)bestDifficulty / (double)0x1000000;
			if( workData.workEntry[0].dataIsValid )
			{
				primeStats.bestPrimeChainDifficultySinceLaunch = max(primeStats.bestPrimeChainDifficultySinceLaunch, primeDifficulty);
				printf("primes/s: %d best difficulty: %f record: %f\n", (sint32)primesPerSecond, (float)primeDifficulty, (float)primeStats.bestPrimeChainDifficultySinceLaunch);
			}
		}
		// wait and check some stats
		uint32 time_updateWork = GetTickCount();
		while( true )
		{
			uint32 passedTime = GetTickCount() - time_updateWork;
			if( passedTime >= 4000 )
				break;
			xptClient_process(workData.xptClient);
			char* disconnectReason = false;
			if( workData.xptClient == NULL || xptClient_isDisconnected(workData.xptClient, &disconnectReason) )
			{
				// disconnected, mark all data entries as invalid
				for(uint32 i=0; i<128; i++)
					workData.workEntry[i].dataIsValid = false;
				printf("xpt: Disconnected, auto reconnect in 30 seconds\n");
				if( workData.xptClient && disconnectReason )
					printf("xpt: Disconnect reason: %s\n", disconnectReason);
				Sleep(30*1000);
				if( workData.xptClient )
					xptClient_free(workData.xptClient);
				xptWorkIdentifier = 0xFFFFFFFF;
				while( true )
				{
					workData.xptClient = xptClient_connect(&jsonRequestTarget, commandlineInput.numThreads);
					if( workData.xptClient )
						break;
				}
			}
			// has the block data changed?
			if( workData.xptClient && xptWorkIdentifier != workData.xptClient->workDataCounter )
			{
				// printf("New work\n");
				xptWorkIdentifier = workData.xptClient->workDataCounter;
				for(uint32 i=0; i<workData.xptClient->payloadNum; i++)
				{
					uint8 blockData[256];
					memset(blockData, 0x00, sizeof(blockData));
					*(uint32*)(blockData+0) = workData.xptClient->blockWorkInfo.version;
					memcpy(blockData+4, workData.xptClient->blockWorkInfo.prevBlock, 32);
					memcpy(blockData+36, workData.xptClient->workData[i].merkleRoot, 32);
					*(uint32*)(blockData+68) = workData.xptClient->blockWorkInfo.nTime;
					*(uint32*)(blockData+72) = workData.xptClient->blockWorkInfo.nBits;
					*(uint32*)(blockData+76) = 0; // nonce
					memcpy(workData.workEntry[i].data, blockData, 80);
					((serverData_t*)workData.workEntry[i].serverData)->blockHeight = workData.xptClient->blockWorkInfo.height;
					((serverData_t*)workData.workEntry[i].serverData)->nBitsForShare = workData.xptClient->blockWorkInfo.nBitsShare;
					// is the data really valid?
					if( workData.xptClient->blockWorkInfo.nTime > 0 )
						workData.workEntry[i].dataIsValid = true;
					else
						workData.workEntry[i].dataIsValid = false;
				}
			}
			Sleep(10);
		}
		loopCounter++;
	}

	return 0;
}

int main(int argc, char **argv)
{
	// setup some default values
	commandlineInput.port = 10034;
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	commandlineInput.numThreads = sysinfo.dwNumberOfProcessors;
	commandlineInput.numThreads = max(commandlineInput.numThreads, 1);
	commandlineInput.sieveSize = 1000000; // default maxSieveSize
	// parse command lines
	jhMiner_parseCommandline(argc, argv);
	// Sets max sieve size
	nMaxSieveSize = commandlineInput.sieveSize;
	if( commandlineInput.host == NULL )
	{
		printf("Missing -o option\n");
		ExitProcess(-1);
	}
	//CRYPTO_set_mem_ex_functions(mallocEx, reallocEx, freeEx);
	
	printf("\n");
	printf("\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n");
	printf("\xBA  jhPrimeMiner (v0.31 beta)                                    \xBA\n");
	printf("\xBA  author: JH (http://ypool.net)                                \xBA\n");
	printf("\xBA  contributors: x3maniac                                       \xBA\n");
	printf("\xBA  Credits: Sunny King for the original Primecoin client&miner  \xBA\n");
	printf("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
	printf("Launching miner...\n");
	// set priority lower so the user still can do other things
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	// init memory speedup (if not already done in preMain)
	//mallocSpeedupInit();
	if( pctx == NULL )
		pctx = BN_CTX_new();
	// init prime table
	GeneratePrimeTable();
	// init winsock
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
	// init critical section
	InitializeCriticalSection(&workData.cs);
	// connect to host
	hostent* hostInfo = gethostbyname(commandlineInput.host);
	if( hostInfo == NULL )
	{
		printf("Cannot resolve '%s'. Is it a valid URL?\n", commandlineInput.host);
		ExitProcess(-1);
	}
	void** ipListPtr = (void**)hostInfo->h_addr_list;
	uint32 ip = 0xFFFFFFFF;
	if( ipListPtr[0] )
	{
		ip = *(uint32*)ipListPtr[0];
	}
	char ipText[32];
	esprintf(ipText, "%d.%d.%d.%d", ((ip>>0)&0xFF), ((ip>>8)&0xFF), ((ip>>16)&0xFF), ((ip>>24)&0xFF));
	if( ((ip>>0)&0xFF) != 255 )
	{
		printf("Connecting to '%s' (%d.%d.%d.%d)\n", commandlineInput.host, ((ip>>0)&0xFF), ((ip>>8)&0xFF), ((ip>>16)&0xFF), ((ip>>24)&0xFF));
	}
	// setup RPC connection data (todo: Read from command line)
	jsonRequestTarget.ip = ipText;
	jsonRequestTarget.port = commandlineInput.port;
	jsonRequestTarget.authUser = commandlineInput.workername;
	jsonRequestTarget.authPass = commandlineInput.workerpass;
	// init stats
	primeStats.primeLastUpdate = GetTickCount();
	// setup thread count and print info
	printf("Using %d threads\n", commandlineInput.numThreads);
	printf("Username: %s\n", jsonRequestTarget.authUser);
	printf("Password: %s\n", jsonRequestTarget.authPass);
	// decide protocol
	if( commandlineInput.port == 10034 )
	{
		// port 10034 indicates xpt protocol (in future we will also add a -o URL prefix)
		workData.protocolMode = MINER_PROTOCOL_XPUSHTHROUGH;
		printf("Using x.pushthrough protocol\n");
	}
	else
	{
		workData.protocolMode = MINER_PROTOCOL_GETWORK;
		printf("Using GetWork() protocol\n");
		printf("Warning: \n");
		printf("   GetWork() is outdated and inefficient. You are losing mining performance\n");
		printf("   by using it. If the pool supports it, consider switching to x.pushthrough.\n");
		printf("   Just add the port :10034 to the -o parameter.\n");
		printf("   Example: jhPrimeminer.exe -o http://poolurl.net:10034 ...\n");
	}
	// initial query new work / create new connection
	if( workData.protocolMode == MINER_PROTOCOL_GETWORK )
	{
		jhMiner_queryWork_primecoin();
	}
	else if( workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH )
	{
		workData.xptClient = NULL;
		// x.pushthrough initial connect & login sequence
		while( true )
		{
			// repeat connect & login until it is successful (with 30 seconds delay)
			while ( true )
			{
				workData.xptClient = xptClient_connect(&jsonRequestTarget, commandlineInput.numThreads);
				if( workData.xptClient != NULL )
					break;
				printf("Failed to connect, retry in 30 seconds\n");
				Sleep(1000*30);
			}
			// make sure we are successfully authenticated
			while( xptClient_isDisconnected(workData.xptClient, NULL) == false && xptClient_isAuthenticated(workData.xptClient) == false )
			{
				xptClient_process(workData.xptClient);
				Sleep(1);
			}
			char* disconnectReason = NULL;
			// everything went alright?
			if( xptClient_isDisconnected(workData.xptClient, &disconnectReason) == true )
			{
				xptClient_free(workData.xptClient);
				workData.xptClient = NULL;
				break;
			}
			if( xptClient_isAuthenticated(workData.xptClient) == true )
			{
				break;
			}
			if( disconnectReason )
				printf("xpt error: %s\n", disconnectReason);
			// delete client
			xptClient_free(workData.xptClient);
			// try again in 30 seconds
			printf("x.pushthrough authentication sequence failed, retry in 30 seconds\n");
			Sleep(30*1000);
		}
	}
	// enter different mainloops depending on protocol mode
	if( workData.protocolMode == MINER_PROTOCOL_GETWORK )
		return jhMiner_main_getworkMode();
	else if( workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH )
		return jhMiner_main_xptMode();

	return 0;
}

///*
// * We need to call this before the actual initialization of the bignum constants in prime.h and other files
// */
//int preMain_initCryptoMemFunctions()
//{
//	//mallocSpeedupInit();
//	/*CRYPTO_set_mem_ex_functions(mallocEx, reallocEx, freeEx);
//	CRYPTO_set_mem_debug_functions(NULL, NULL, NULL, NULL, NULL);*/
//	// See comment above mallocEx() method
//	return 0;
//}
//
//typedef int cb(void);
//
//#pragma data_seg(".CRT$XIU")
//static cb *autostart[] = { preMain_initCryptoMemFunctions };
//
//#pragma data_seg() /* reset data-segment */

void debug_getSieveDataHash(CSieveOfEratosthenes* sieve, uint8* hashOut)
{
	sha256_context ctx;
	sha256_starts(&ctx);
	sha256_update(&ctx, (uint8*)sieve->vfCompositeCunningham1, (nMaxSieveSize+7)/8);
	sha256_update(&ctx, (uint8*)sieve->vfCompositeCunningham2, (nMaxSieveSize+7)/8);
	sha256_update(&ctx, (uint8*)sieve->vfCompositeBiTwin, (nMaxSieveSize+7)/8);
	sha256_finish(&ctx, hashOut);
}

int mainPerformanceTest()
{
	GeneratePrimeTable();
	// performance test for sieve generation
	sint32 sieveSize = 1000000;
	uint32 nBits = 0x07fb8bcc;
	uint256 blockHashHeader;
	yPoolWorkMgr_parseHexString("eed69c071ac2634ffc2a9e73177d1c5fad92fdf06f6d711c2f04877906ad6aef", 32*2, blockHashHeader.begin());
	CBigNum fixedMultiplier = CBigNum(0xB);

	uint8 orgSieveHash[32];
	uint8 fastSieveHash[32];

	printf("Generating original sieve and fast sieve...\n");
	uint32 time1 = GetTickCount();
	CSieveOfEratosthenes* originalSieve = new CSieveOfEratosthenes(sieveSize, nBits, blockHashHeader, fixedMultiplier);
	while (originalSieve->WeaveOriginal() );
	uint32 time2 = GetTickCount();
	printf("Original sieve time: %8dms Hash: ", time2-time1);
	debug_getSieveDataHash(originalSieve, orgSieveHash);
	for(uint32 i=0; i<12; i++)
		printf("%02x", orgSieveHash[i]);
	puts("");
	puts("Start generating fast sieve...\n");
	uint32 time3 = GetTickCount();
	CSieveOfEratosthenes* fastSieve = new CSieveOfEratosthenes(sieveSize, nBits, blockHashHeader, fixedMultiplier);
	//while (fastSieve->WeaveFast2() );
	fastSieve->WeaveFastAll();
	uint32 time4 = GetTickCount();
	printf("Fast sieve time:     %8dms Hash: ", time4-time3);
	debug_getSieveDataHash(fastSieve, fastSieveHash);
	for(uint32 i=0; i<12; i++)
		printf("%02x", fastSieveHash[i]);
	puts("");
	while( true ) Sleep(1000);
	return 0;
}
