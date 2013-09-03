#include"global.h"

#include<intrin.h>
#include<ctime>

typedef struct  
{
	char* workername;
	char* workerpass;
	char* host;
	sint32 port;
	sint32 numThreads;
	sint32 sieveSize;
	sint32 sievePrimeLimit;	// how many primes should be sieved
	// GPU / OpenCL options

	// mode option
	uint32 mode;
}commandlineInput_t;

#define MINER_MODE_XPM_DEFAULT			0	// use the optimized original code
#define MINER_MODE_XPM_MULTIPASSSIEVE	1	// use the v0.4 sieve method
#define MINER_MODE_XPM_OPENCL			2	// use OpenCL for sieving, CPU for chain test

commandlineInput_t commandlineInput = {0};

primeStats_t primeStats = {0};
volatile int total_shares = 0;
volatile int valid_shares = 0;
char* dt;

minerSettings_t minerSettings = {0};

// default good settings:
#define DEFAULT_SIEVE_SIZE			(1024*2000)
#define DEFAULT_PRIMES_TO_SIEVE		(28000)

char* minerVersionString = "jhPrimeminer v0.5 r5 (official)"; // this is the version string displayed on the worker live stats page (max 45 characters)

bool error(const char *format, ...)
{
	puts(format);
	return false;
}

// Important: Once you are done with the fStr method

bool hex2bin(unsigned char *p, const char *hexstr, size_t len)
{
	bool ret = false;

	while (*hexstr && len) {
		char hex_byte[4];
		unsigned int v;

		if (!hexstr[1]) {
			printf("hex2bin str truncated");
			return ret;
		}

		memset(hex_byte, 0, 4);
		hex_byte[0] = hexstr[0];
		hex_byte[1] = hexstr[1];

		if (sscanf(hex_byte, "%x", &v) != 1) {
			printf( "hex2bin sscanf '%s' failed", hex_byte);
			return ret;
		}

		*p = (unsigned char) v;

		p++;
		hexstr += 2;
		len--;
	}

	if (len == 0 && *hexstr == 0)
		ret = true;
	return ret;
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
	CBigNum bnPrimeChainMultiplier;
	bnPrimeChainMultiplier.SetHex(primecoinBlock->mpzPrimeChainMultiplier.get_str(16));
	std::vector<unsigned char> bnSerializeData = bnPrimeChainMultiplier.getvch();
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
	uint32 timestamp;
	uint32 numberOfProcessedSieves;
	uint32 proofOfWork_chainLength;
	uint8  proofOfWork_chainType;
	uint32 proofOfWork_multiplier;
	uint32 proofOfWork_nonce;
	uint32 proofOfWork_depth;
	uint32 nonceEnd;
}xpmScannedNonceRange_t;

typedef struct  
{
	uint8 merkleRoot[32];
	uint8 prevBlockHash[32];
	uint32 timestampStart;
	uint32 sieveSize;
	uint32 primesToSieve;
	// note: Server constraints like nonceMin/nonceMax dont need to be send to the server
	std::vector<xpmScannedNonceRange_t> scannedRange; // list of scanned range
	bool finished; // when set to true, the data is ready to be sent
}xpmProofOfWork_t;

typedef struct  
{
	CRITICAL_SECTION cs;
	uint8 protocolMode;
	// xpm
	xptBlockWorkInfo_t workEntryQueue[256]; // work data for each thread (up to 2 per thread)
	uint32 workEntrySize; // number of queued elements
	// x.pushthrough
	xptClient_t* xptClient;
	// proof of work
	// note: We sort proofOfWork data not by threadId, but instead by merkleRoot, so we can also handle special test data issued by the server in the same struct
	std::vector<xpmProofOfWork_t> proofOfWork;
	uint32 mostRecentBlockHeight; // updated whenever we receive new data
	//xpmProofOfWork_t proofOfWork[128]; // proof of work info for each thread (up to 128, like workEntry)
}workData_t;



#define MINER_PROTOCOL_GETWORK		(1)
#define MINER_PROTOCOL_STRATUM		(2)
#define MINER_PROTOCOL_XPUSHTHROUGH	(3)

workData_t workData = {0};

jsonRequestTarget_t jsonRequestTarget; // rpc login data
jsonRequestTarget_t jsonLocalPrimeCoin; // rpc login data
bool useLocalPrimecoindForLongpoll;


/*
 * Initializes a new proof of work chain (same merkleRoot and prevBlockHash)
 */
void jhMiner_primecoinBeginProofOfWork(uint8 merkleRoot[32], uint8 prevBlockHash[32], uint32 sieveSize, uint32 primesToSieve, uint32 timestampStart)
{
	//printf("Starting proof of work hash...\n");
	xpmProofOfWork_t proofOfWork = {0};
	memcpy(proofOfWork.merkleRoot, merkleRoot, 32);
	memcpy(proofOfWork.prevBlockHash, prevBlockHash, 32);
	proofOfWork.timestampStart = timestampStart;
	proofOfWork.sieveSize = sieveSize;
	proofOfWork.primesToSieve = primesToSieve;
	EnterCriticalSection(&workData.cs);
	workData.proofOfWork.push_back(proofOfWork);
	LeaveCriticalSection(&workData.cs);
}

/*
 * Adds the result of the processed work (proof of work) to the local queued work results for later transmission to server
 */
void jhMiner_primecoinAddProofOfWork(uint8 merkleRoot[32], uint8 prevBlockHash[32], uint32 numberOfProcessedSieves, uint32 timestamp, uint32 proofChainLength, uint8 proofChainType, uint32 proofNonce, uint32 proofMultiplier, uint32 proofDepth)
{
	// printf("Feeding proof of work hash, timestamp: %08X sieves: %d proof: %08x mp: %d\n", timestamp, numberOfProcessedSieves, proofChainLength, proofMultiplier);
	EnterCriticalSection(&workData.cs);
	for(uint32 i=0; i<workData.proofOfWork.size(); i++)
	{
		if( memcmp(workData.proofOfWork[i].prevBlockHash, prevBlockHash, 32) == 0 && memcmp(workData.proofOfWork[i].merkleRoot, merkleRoot, 32) == 0 )
		{
			xpmScannedNonceRange_t xpmScannedNonceRange = {0};
			xpmScannedNonceRange.proofOfWork_chainLength = proofChainLength;
			xpmScannedNonceRange.proofOfWork_chainType = proofChainLength;
			xpmScannedNonceRange.proofOfWork_nonce = proofNonce;
			xpmScannedNonceRange.proofOfWork_multiplier = proofMultiplier;
			xpmScannedNonceRange.proofOfWork_depth = proofDepth;
			xpmScannedNonceRange.numberOfProcessedSieves = numberOfProcessedSieves;
			xpmScannedNonceRange.timestamp = timestamp;
			xpmScannedNonceRange.nonceEnd = 0xFFFFFFFF; // full range scan
			workData.proofOfWork[i].scannedRange.push_back(xpmScannedNonceRange);
			break;
		}
	}
	LeaveCriticalSection(&workData.cs);
}

void jhMiner_primecoinCompleteProofOfWork(uint8 merkleRoot[32], uint8 prevBlockHash[32], uint32 nonceEnd, uint32 numberOfProcessedSieves, uint32 timestamp, uint32 proofChainLength, uint8 proofChainType, uint32 proofNonce, uint32 proofMultiplier, uint32 proofDepth)
{
	//printf("Finishing proof of work hash, nonceEnd: %08X sieves: %d\n", nonceEnd, numberOfProcessedSieves);
	EnterCriticalSection(&workData.cs);
	for(uint32 i=0; i<workData.proofOfWork.size(); i++)
	{
		if( memcmp(workData.proofOfWork[i].prevBlockHash, prevBlockHash, 32) == 0 && memcmp(workData.proofOfWork[i].merkleRoot, merkleRoot, 32) == 0 )
		{
			xpmScannedNonceRange_t xpmScannedNonceRange = {0};
			xpmScannedNonceRange.proofOfWork_chainLength = proofChainLength;
			xpmScannedNonceRange.proofOfWork_chainType = proofChainLength;
			xpmScannedNonceRange.proofOfWork_nonce = proofNonce;
			xpmScannedNonceRange.proofOfWork_multiplier = proofMultiplier;
			xpmScannedNonceRange.proofOfWork_depth = proofDepth;
			xpmScannedNonceRange.numberOfProcessedSieves = numberOfProcessedSieves;
			xpmScannedNonceRange.timestamp = timestamp;
			xpmScannedNonceRange.nonceEnd = nonceEnd;
			workData.proofOfWork[i].scannedRange.push_back(xpmScannedNonceRange);
			workData.proofOfWork[i].finished = true;
			break;
		}
	}
	LeaveCriticalSection(&workData.cs);
}

/*
 * Called periodically (about once a minute) to send the current collected and finished proof of work data to the server
 * Must be called by the same thread that also manages the xptClient struct
 */
void jhMiner_primecoinSendProofOfWork()
{
	if( workData.xptClient == NULL )
		return; // not connected
	EnterCriticalSection(&workData.cs);
	// count how many PoW blocks we have to send
	uint32 numPoW = 0;
	for(uint32 i=0; i<workData.proofOfWork.size(); i++)
	{
		if( workData.proofOfWork[i].finished )
			numPoW++;
	}
	if( numPoW == 0 )
	{
		// nothing to send, quit early
		LeaveCriticalSection(&workData.cs);
		return;
	}
	// start building packet
	bool sendError = false;
	xptClient_t* xptClient = workData.xptClient;
	xptPacketbuffer_beginWritePacket(xptClient->sendBuffer, XPT_OPC_C_SUBMIT_POW);
	// write number of PoWs that follow
	xptPacketbuffer_writeU16(xptClient->sendBuffer, &sendError, numPoW);
	// write PoW data
	for(uint32 i=0; i<workData.proofOfWork.size(); i++)
	{
		if( workData.proofOfWork[i].finished == false )
			continue;
		xpmProofOfWork_t* proofOfWork = &workData.proofOfWork[i];
		// assert(proofOfWork->scannedRange.size() == 0 );
		uint32 numCompletedNonceRange = proofOfWork->scannedRange.size() - 1;
		xptPacketbuffer_writeU16(xptClient->sendBuffer, &sendError, numCompletedNonceRange);
		// general info related to PoW
		xptPacketbuffer_writeData(xptClient->sendBuffer, proofOfWork->prevBlockHash, 32, &sendError);
		xptPacketbuffer_writeData(xptClient->sendBuffer, proofOfWork->merkleRoot, 32, &sendError);
		xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, proofOfWork->timestampStart);
		xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, proofOfWork->sieveSize);
		xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, proofOfWork->primesToSieve);
		xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, workData.proofOfWork[i].scannedRange[numCompletedNonceRange].nonceEnd);
		for(uint32 f=0; f<numCompletedNonceRange+1; f++)
		{
			// todo: Find a way to compress this information
			xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, workData.proofOfWork[i].scannedRange[f].proofOfWork_chainLength);
			xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, workData.proofOfWork[i].scannedRange[f].proofOfWork_multiplier);
			xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, workData.proofOfWork[i].scannedRange[f].proofOfWork_nonce);
			xptPacketbuffer_writeU8(xptClient->sendBuffer, &sendError, workData.proofOfWork[i].scannedRange[f].proofOfWork_depth);
			xptPacketbuffer_writeU8(xptClient->sendBuffer, &sendError, workData.proofOfWork[i].scannedRange[f].proofOfWork_chainType);
		}
	}
	// delete all processed PoW
	for (std::vector<xpmProofOfWork_t>::iterator it = workData.proofOfWork.begin() ; it != workData.proofOfWork.end();)
	{
		if( it->finished )
			it = workData.proofOfWork.erase(it);
		else
			it++;
	}
	// finalize packet
	xptPacketbuffer_finalizeWritePacket(xptClient->sendBuffer);
	// send to server
	printf("PoW send: %d bytes\n", xptClient->sendBuffer->parserIndex);
	send(xptClient->clientSocket, (const char*)(xptClient->sendBuffer->buffer), xptClient->sendBuffer->parserIndex, 0);

	LeaveCriticalSection(&workData.cs);
}

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
		// disabled
		//// prepare buffer to send
		//fStr_buffer4kb_t fStrBuffer_parameter;
		//fStr_t* fStr_parameter = fStr_alloc(&fStrBuffer_parameter, FSTR_FORMAT_UTF8);
		//fStr_append(fStr_parameter, "[\""); // \"]
		//fStr_addHexString(fStr_parameter, data, 256);
		//fStr_appendFormatted(fStr_parameter, "\",\"");
		//fStr_addHexString(fStr_parameter, (uint8*)&primecoinBlock->serverData, 32);
		//fStr_append(fStr_parameter, "\"]");
		//// send request
		//sint32 rpcErrorCode = 0;
		//jsonObject_t* jsonReturnValue = jsonClient_request(&jsonRequestTarget, "getwork", fStr_parameter, &rpcErrorCode);
		//if( jsonReturnValue == NULL )
		//{
		//	printf("PushWorkResult failed :(\n");
		//	return false;
		//}
		//else
		//{
		//	// rpc call worked, sooooo.. is the server happy with the result?
		//	jsonObject_t* jsonReturnValueBool = jsonObject_getParameter(jsonReturnValue, "result");
		//	if( jsonObject_isTrue(jsonReturnValueBool) )
		//	{
		//		total_shares++;
		//		valid_shares++;
		//		time_t now = time(0);
		//		dt = ctime(&now);
		//		//printf("Valid share found!");
		//		//printf("[ %d / %d ] %s",valid_shares, total_shares,dt);
		//		jsonObject_freeObject(jsonReturnValue);
		//		return true;
		//	}
		//	else
		//	{
		//		total_shares++;
		//		// the server says no to this share :(
		//		printf("Server rejected share (BlockHeight: %d/%d nBits: 0x%08X)\n", primecoinBlock->serverData.blockHeight, jhMiner_getCurrentWorkBlockHeight(primecoinBlock->threadIndex), primecoinBlock->serverData.client_shareBits);
		//		jsonObject_freeObject(jsonReturnValue);
		//		return false;
		//	}
		//}
		//jsonObject_freeObject(jsonReturnValue);
		//return false;
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
		CBigNum bnPrimeChainMultiplier;
		bnPrimeChainMultiplier.SetHex(primecoinBlock->mpzPrimeChainMultiplier.get_str(16));
		std::vector<unsigned char> bnSerializeData = bnPrimeChainMultiplier.getvch();
		sint32 lengthBN = (sint32)bnSerializeData.size();
		memcpy(xptShareToSubmit->chainMultiplier, &bnSerializeData[0], lengthBN);
		xptShareToSubmit->chainMultiplierSize = lengthBN;
		// todo: Set stuff like sieve size
		if( workData.xptClient && !workData.xptClient->disconnected)
			xptClient_foundShare(workData.xptClient, xptShareToSubmit);
		else
		{
			printf("Share submission failed. The client is not connected to the pool.");
		}
	}
	return true;
}

int queryLocalPrimecoindBlockCount(bool useLocal)
{
	sint32 rpcErrorCode = 0;
	jsonObject_t* jsonReturnValue = jsonClient_request(useLocal ? &jsonLocalPrimeCoin : &jsonRequestTarget, "getblockcount", NULL, &rpcErrorCode);
	if( jsonReturnValue == NULL )
	{
		printf("getblockcount() failed with %serror code %d\n", (rpcErrorCode>1000)?"http ":"", rpcErrorCode>1000?rpcErrorCode-1000:rpcErrorCode);
		return 0;
	}
	else
	{
		jsonObject_t* jsonResult = jsonObject_getParameter(jsonReturnValue, "result");
		return (int) jsonObject_getNumberValueAsS32(jsonResult);
		jsonObject_freeObject(jsonReturnValue);
	}

	return 0;
}

static double DIFFEXACTONE = 26959946667150639794667015087019630673637144422540572481103610249215.0;
static const uint64_t diffone = 0xFFFF000000000000ull;

static double target_diff(const unsigned char *target)
{
	double targ = 0;
	signed int i;

	for (i = 31; i >= 0; --i)
		targ = (targ * 0x100) + target[i];

	return DIFFEXACTONE / (targ ? targ: 1);
}


//static double DIFFEXACTONE = 26959946667150639794667015087019630673637144422540572481103610249215.0;
//static const uint64_t diffone = 0xFFFF000000000000ull;

double target_diff(const uint32_t  *target)
{
	double targ = 0;
	signed int i;

	for (i = 0; i < 8; i++)
		targ = (targ * 0x100) + target[7 - i];

	return DIFFEXACTONE / ((double)targ ?  targ : 1);
}


std::string HexBits(unsigned int nBits)
{
    union {
        int32_t nBits;
        char cBits[4];
    } uBits;
    uBits.nBits = htonl((int32_t)nBits);
    return HexStr(BEGIN(uBits.cBits), END(uBits.cBits));
}
void jhMiner_queryWork_primecoin()
{
	// disabled
	//sint32 rpcErrorCode = 0;
	//uint32 time1 = GetTickCount();
	//jsonObject_t* jsonReturnValue = jsonClient_request(&jsonRequestTarget, "getwork", NULL, &rpcErrorCode);
	//uint32 time2 = GetTickCount() - time1;
	//// printf("request time: %dms\n", time2);
	//if( jsonReturnValue == NULL )
	//{
	//	printf("Getwork() failed with %serror code %d\n", (rpcErrorCode>1000)?"http ":"", rpcErrorCode>1000?rpcErrorCode-1000:rpcErrorCode);
	//	workData.workEntry[0].dataIsValid = false;
	//	return;
	//}
	//else
	//{
	//	jsonObject_t* jsonResult = jsonObject_getParameter(jsonReturnValue, "result");
	//	jsonObject_t* jsonResult_data = jsonObject_getParameter(jsonResult, "data");
	//	//jsonObject_t* jsonResult_hash1 = jsonObject_getParameter(jsonResult, "hash1");
	//	jsonObject_t* jsonResult_target = jsonObject_getParameter(jsonResult, "target");
	//	jsonObject_t* jsonResult_serverData = jsonObject_getParameter(jsonResult, "serverData");
	//	//jsonObject_t* jsonResult_algorithm = jsonObject_getParameter(jsonResult, "algorithm");
	//	if( jsonResult_data == NULL )
	//	{
	//		printf("Error :(\n");
	//		workData.workEntry[0].dataIsValid = false;
	//		jsonObject_freeObject(jsonReturnValue);
	//		return;
	//	}
	//	// data
	//	uint32 stringData_length = 0;
	//	uint8* stringData_data = jsonObject_getStringData(jsonResult_data, &stringData_length);
	//	//printf("data: %.*s...\n", (sint32)min(48, stringData_length), stringData_data);

	//	EnterCriticalSection(&workData.cs);
	//	yPoolWorkMgr_parseHexString((char*)stringData_data, min(128*2, stringData_length), workData.workEntry[0].data);
	//	workData.workEntry[0].dataIsValid = true;
	//	// get server data
	//	uint32 stringServerData_length = 0;
	//	uint8* stringServerData_data = jsonObject_getStringData(jsonResult_serverData, &stringServerData_length);
	//	RtlZeroMemory(workData.workEntry[0].serverData, 32);
	//	if( jsonResult_serverData )
	//		yPoolWorkMgr_parseHexString((char*)stringServerData_data, min(128*2, 32*2), workData.workEntry[0].serverData);
	//	// generate work hash
	//	uint32 workDataHash = 0x5B7C8AF4;
	//	for(uint32 i=0; i<stringData_length/2; i++)
	//	{
	//		workDataHash = (workDataHash>>29)|(workDataHash<<3);
	//		workDataHash += (uint32)workData.workEntry[0].data[i];
	//	}
	//	workData.workEntry[0].dataHash = workDataHash;
	//	LeaveCriticalSection(&workData.cs);
	//	jsonObject_freeObject(jsonReturnValue);
	//}
}

/*
 * Returns the block height of the most recently received workload
 */
uint32 jhMiner_getCurrentWorkBlockHeight(sint32 threadIndex)
{
	return workData.mostRecentBlockHeight;	
}

/*
 * Called to initialize a mode (before worker threads are started)
 */
void jhMiner_initMode()
{
	if( commandlineInput.mode == MINER_MODE_XPM_OPENCL )
	{
		openCL_init();

	}
	// other modes dont require special initialization
}

/*
 * Called whenever new work is there (or the previous work finished processing)
 */
void BitcoinMiner_modeSwitch(primecoinBlock_t* primecoinBlock, sint32 threadIndex)
{
	if( commandlineInput.mode == MINER_MODE_XPM_OPENCL )
		BitcoinMiner_openCL(primecoinBlock, threadIndex);
	else if( commandlineInput.mode == MINER_MODE_XPM_MULTIPASSSIEVE )
		BitcoinMiner_multipassSieve(primecoinBlock, threadIndex);
	else if( commandlineInput.mode == MINER_MODE_XPM_DEFAULT )
		BitcoinMiner(primecoinBlock, threadIndex);
}


/*
 * Worker thread mainloop for getwork() mode
 */
int jhMiner_workerThread_getwork(int threadIndex)
{
	// disabled
	//mallocSpeedupInitPerThread();
	//while( true )
	//{
	//	uint8 localBlockData[128];
	//	// copy block data from global workData
	//	uint32 workDataHash = 0;
	//	uint8 serverData[32];
	//	while( workData.workEntry[0].dataIsValid == false ) Sleep(200);
	//	EnterCriticalSection(&workData.cs);
	//	memcpy(localBlockData, workData.workEntry[0].data, 128);
	//	//seed = workData.seed;
	//	memcpy(serverData, workData.workEntry[0].serverData, 32);
	//	LeaveCriticalSection(&workData.cs);
	//	// swap endianess
	//	for(uint32 i=0; i<128/4; i++)
	//	{
	//		*(uint32*)(localBlockData+i*4) = _swapEndianessU32(*(uint32*)(localBlockData+i*4));
	//	}
	//	// convert raw data into primecoin block
	//	primecoinBlock_t primecoinBlock = {0};
	//	memcpy(&primecoinBlock, localBlockData, 80);
	//	// we abuse the timestamp field to generate an unique hash for each worker thread...
	//	primecoinBlock.timestamp += threadIndex;
	//	primecoinBlock.threadIndex = threadIndex;
	//	primecoinBlock.xptMode = (workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH);
	//	// ypool uses a special encrypted serverData value to speedup identification of merkleroot and share data
	//	memcpy(&primecoinBlock.serverData, serverData, 32);
	//	// start mining
	//	BitcoinMiner_modeSwitch(&primecoinBlock, threadIndex);
	//	primecoinBlock.mpzPrimeChainMultiplier = 0;
	//}
	return 0;
}

/*
 * Worker thread mainloop for xpt() mode
 */
int jhMiner_workerThread_xpt(int threadIndex)
{
	mallocSpeedupInitPerThread();
	while( true )
	{
		//uint8 localBlockData[128];
		// init primecoin block
		primecoinBlock_t primecoinBlock = {0};
		//memcpy(&primecoinBlock, localBlockData, 80);
		// copy block data from global workData
		uint32 workDataHash = 0;
		while( true )
		{
			Sleep(50);
			EnterCriticalSection(&workData.cs);
			if( workData.workEntrySize > 0 )
				break;
			LeaveCriticalSection(&workData.cs);
		}
		//memcpy(localBlockData, workData.workEntry[threadIndex].data, 128);
		//memcpy(serverData, workData.workEntry[threadIndex].serverData, 32);
		//memcpy(&primecoinBlock.serverData, serverData, 32);
		//workDataHash = workData.workEntry[threadIndex].dataHash;
		//

		
		//memcpy(workData.workEntryQueue+workData.workEntrySize, workData.xptClient->blockWorkInfo);
		
		// do not process the top element (as it is the most recent one) but instead pick a random one
		uint32 rdProcessIdx = (rand()%workData.workEntrySize);
		workData.workEntrySize--; // remove top element
		xptBlockWorkInfo_t* blockInputData = &workData.workEntryQueue[rdProcessIdx];//workData.workEntrySize;
		primecoinBlock.blockHeight = blockInputData->height;
		primecoinBlock.nBits = blockInputData->nBits;
		primecoinBlock.nBitsForShare = blockInputData->nBitsShare;
		primecoinBlock.version = blockInputData->version;
		primecoinBlock.timestamp = blockInputData->nTime;
		memcpy(primecoinBlock.merkleRoot, blockInputData->merkleRoot, 32);
		memcpy(primecoinBlock.prevBlockHash, blockInputData->prevBlockHash, 32);

		primecoinBlock.fixedPrimorial = blockInputData->fixedPrimorial;
		primecoinBlock.fixedHashFactor = blockInputData->fixedHashFactor;
		primecoinBlock.sievesizeMin = blockInputData->sievesizeMin;
		primecoinBlock.sievesizeMax = blockInputData->sievesizeMax;
		primecoinBlock.primesToSieveMin = blockInputData->primesToSieveMin;
		primecoinBlock.primesToSieveMax = blockInputData->primesToSieveMax;
		primecoinBlock.sieveChainLength = blockInputData->sieveChainLength;
		primecoinBlock.nonceMin = blockInputData->nonceMin;
		primecoinBlock.nonceMax = blockInputData->nonceMax;
		primecoinBlock.xptFlags = blockInputData->flags;

		// shift old top work element to new empty location
		if( workData.workEntrySize > 0 )
			memcpy(blockInputData, &workData.workEntryQueue[workData.workEntrySize], sizeof(xptBlockWorkInfo_t));

		LeaveCriticalSection(&workData.cs);
		// local data for the block
		primecoinBlock.threadIndex = threadIndex;
		primecoinBlock.xptMode = (workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH);
		// start mining
		BitcoinMiner_modeSwitch(&primecoinBlock, threadIndex);
		primecoinBlock.mpzPrimeChainMultiplier = 0;
	}
	return 0;
}

void jhMiner_printHelp()
{
	puts("Usage: jhPrimeminer.exe [options]");
	puts("Options:");
	puts("   -o, -O                        The miner will connect to this url");
	puts("                                     You can specifiy an port after the url using -o url:port");
	puts("   -u                            The username (workername) used for login");
	puts("   -p                            The password used for login");
	puts("   -t <num>                      The number of threads for mining (default 1)");
	puts("                                     For most efficient mining, set to number of CPU cores");
	puts("   -s <num>                      Set MaxSieveSize range from 200000 - 10000000");
	printf("                                     Default is %d.\n", DEFAULT_SIEVE_SIZE);
	//puts("   -d <num>                      Set SievePercentage - range from 1 - 1200");
	//printf("                                     Default is %d. It's not recommended to use lower values than 10.\n", DEFAULT_SIEVE_PERCENTAGE);
	//puts("                                     It limits how many base primes are used to filter out candidate multipliers in the sieve.");
	puts("   -primes <num>                 Sets how many prime factors are used to filter the sieve");
	puts("                                     Default is MaxSieveSize. Valid range: 300 - 2000000");
	printf("                                     Default is %d.\n", DEFAULT_PRIMES_TO_SIEVE);
	puts("   -mode <mode>                  Sets the algorithm used for mining.");
	puts("                                     Default is multipass. Allowed values:");
	puts("                                     default: Use optimized original code");
	puts("                                     multipass: Use experimental multipass sieve");
	//puts("                                     gpusieve: Use OpenCL for the sieve, chain test on CPU");
	puts("Example usage:");
	puts("   jhPrimeminer.exe -o http://poolurl.com:10034 -u workername.1 -p workerpass -t 4");
}

void jhMiner_parseCommandline(int argc, char **argv)
{
	sint32 cIdx = 1;
	while( cIdx < argc )
	{
		char* argument = argv[cIdx];
		cIdx++;
		if( memcmp(argument, "-o", 3)==0 || memcmp(argument, "-O", 3)==0 )
		{
			// -o
			if( cIdx >= argc )
			{
				printf("Missing URL after -o option\n");
				exit(0);
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
		else if( memcmp(argument, "-u", 3)==0 )
		{
			// -u
			if( cIdx >= argc )
			{
				printf("Missing username/workername after -u option\n");
				exit(0);
			}
			commandlineInput.workername = fStrDup(argv[cIdx], 64);
			cIdx++;
		}
		else if( memcmp(argument, "-p", 3)==0 )
		{
			// -p
			if( cIdx >= argc )
			{
				printf("Missing password after -p option\n");
				exit(0);
			}
			commandlineInput.workerpass = fStrDup(argv[cIdx], 64);
			cIdx++;
		}
		else if( memcmp(argument, "-t", 3)==0 )
		{
			// -t
			if( cIdx >= argc )
			{
				printf("Missing thread number after -t option\n");
				exit(0);
			}
			commandlineInput.numThreads = atoi(argv[cIdx]);
			if( commandlineInput.numThreads < 1 || commandlineInput.numThreads > 128 )
			{
				printf("-t parameter out of range");
				exit(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-s", 3)==0 )
		{
			// -s
			if( cIdx >= argc )
			{
				printf("Missing number after -s option\n");
				exit(0);
			}
			commandlineInput.sieveSize = atoi(argv[cIdx]);
			if( commandlineInput.sieveSize < 200000 || commandlineInput.sieveSize > 200000000 )
			{
				printf("-s parameter out of range, must be between 200000 - 200000000");
				exit(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-primes", 7)==0 )
		{
			// -primes
			if( cIdx >= argc )
			{
				printf("Missing number after -primes option\n");
				exit(0);
			}
			commandlineInput.sievePrimeLimit = atoi(argv[cIdx]);
			if( commandlineInput.sievePrimeLimit < 300 || commandlineInput.sievePrimeLimit > 2000000 )
			{
				printf("-primes parameter out of range, must be between 300 - 2000000");
				exit(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-mode", 6)==0 || memcmp(argument, "--mode", 7)==0 )
		{
			// -mode
			if( cIdx >= argc )
			{
				printf("Missing mode specifier after -mode option\n");
				exit(0);
			}
			char* modeVal = argv[cIdx];
			if( strcmp(modeVal, "default")==0 )
				commandlineInput.mode = MINER_MODE_XPM_DEFAULT;
			else if( strcmp(modeVal, "multipass")==0 )
				commandlineInput.mode = MINER_MODE_XPM_MULTIPASSSIEVE;
			else if( strcmp(modeVal, "gpusieve")==0 )
				commandlineInput.mode = MINER_MODE_XPM_OPENCL;
			else
			{
				printf("-mode parameter value unknown.");
				exit(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-help", 6)==0 || memcmp(argument, "--help", 7)==0 )
		{
			jhMiner_printHelp();
			exit(0);
		}
		else
		{
			printf("'%s' is an unknown option.\nType jhPrimeminer.exe --help for more info\n", argument); 
			exit(-1);
		}
	}
	if( argc <= 1 )
	{
		jhMiner_printHelp();
		exit(0);
	}
}

/*
 * Mainloop when using getwork() mode
 */
int jhMiner_main_getworkMode()
{
	jhMiner_initMode();
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
			if( workData.workEntrySize > 0 )
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

bool fIncrementPrimorial = true;

uint32 timeLastAutoAdjust = 0;

typedef struct  
{
	sint32 nSieveSize;
	sint32 nPrimesToSieve;
	// other auto tune vars
	uint32 seed;
	sint32 seedCount;
}autoAdjustSettingsBackup_t;

autoAdjustSettingsBackup_t settingsBackup;

void AutoAdjustSettings()
{
	// removed for now
}


/*
 * Mainloop when using xpt mode
 */
int jhMiner_main_xptMode()
{
	jhMiner_initMode();
	// start threads
	for(sint32 threadIdx=0; threadIdx<commandlineInput.numThreads; threadIdx++)
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)jhMiner_workerThread_xpt, (LPVOID)threadIdx, 0, 0);
	// main thread, don't query work, just wait and process
	sint32 loopCounter = 0;
	uint32 time_multiAdjust = GetTickCount();
	uint32 time_powSend = GetTickCount();

	while( true )
	{
		// calculate stats every second tick
		if( loopCounter&1 )
		{
			double statsPassedTime = (double)(GetTickCount() - primeStats.primeLastUpdate);
			if( statsPassedTime < 1.0 )
				statsPassedTime = 1.0; // avoid division by zero
			double numbersTestedPerSec = (double)primeStats.numAllTestedNumbers / (statsPassedTime / 1000.0);
			numbersTestedPerSec *= 0.001;
			uint32 bestDifficulty = primeStats.bestPrimeChainDifficulty;
			primeStats.bestPrimeChainDifficulty = 0;
			double primeDifficulty = (double)bestDifficulty / (double)0x1000000;
			if( workData.xptClient != NULL ) // dont print info when disconnected
			{
				primeStats.bestPrimeChainDifficultySinceLaunch = max(primeStats.bestPrimeChainDifficultySinceLaunch, primeDifficulty);
				double fourChPerHour = ((double)primeStats.fourChainCount / statsPassedTime) * 3600000.0;
				double fiveChPerHour = ((double)primeStats.fiveChainCount / statsPassedTime) * 3600000.0;
				double sixChPerHour = ((double)primeStats.sixChainCount / statsPassedTime) * 3600000.0;
				double sevenChPerHour = ((double)primeStats.sevenChainCount / statsPassedTime) * 3600000.0;
				float shareValuePerHour = (float)(primeStats.fShareValue / statsPassedTime * 3600000.0);
				printf("Val/h %.04f 4ch/h: %.f 5ch/h: %.f 6ch/h: %.02f 7ch/h: %.02f Num/s: %dk\n", 
					shareValuePerHour, fourChPerHour, fiveChPerHour, sixChPerHour, sevenChPerHour, (sint32)numbersTestedPerSec);
			}
		}
		// wait and check some stats
		uint32 time_updateWork = GetTickCount();
		while( true )
		{
			uint32 tickCount = GetTickCount();
			uint32 passedTime = tickCount - time_updateWork;


			if (tickCount - time_multiAdjust >= 60000)
			{
				AutoAdjustSettings();
				time_multiAdjust = GetTickCount();
			}

			if( tickCount - time_powSend >= (70*1000) )
			{
				jhMiner_primecoinSendProofOfWork();
				time_powSend = GetTickCount() - (rand()%20)*1000; // make sure not every miner sends at the same time
			}
			if( passedTime >= 4000 )
				break;
			xptClient_process(workData.xptClient);
			char* disconnectReason = false;
			if( workData.xptClient == NULL || xptClient_isDisconnected(workData.xptClient, &disconnectReason) )
			{
				// disconnected, mark all data entries as invalid
				workData.workEntrySize = 0;
				workData.mostRecentBlockHeight = 0;
				printf("xpt: Disconnected, auto reconnect in 45 seconds\n");
				if( workData.xptClient && disconnectReason )
					printf("xpt: Disconnect reason: %s\n", disconnectReason);
				Sleep(45*1000);
				if( workData.xptClient )
					xptClient_free(workData.xptClient);
				while( true )
				{
					workData.xptClient = xptClient_connect(&jsonRequestTarget, commandlineInput.numThreads);
					if( workData.xptClient )
						break;
				}
			}
			// new block data in the queue
			if( workData.xptClient && workData.xptClient->blockWorkSize > 0 )
			{
				EnterCriticalSection(&workData.cs);
				EnterCriticalSection(&workData.xptClient->cs_workAccess);
				for(uint32 i=0; i<workData.xptClient->blockWorkSize; i++)
				{
					// create copy of the block
					memcpy(&workData.workEntryQueue[workData.workEntrySize], (workData.xptClient->blockWorkInfo+i), sizeof(xptBlockWorkInfo_t));
					workData.workEntrySize++;
					workData.mostRecentBlockHeight = max(workData.mostRecentBlockHeight, workData.workEntryQueue[workData.workEntrySize-1].height);
				}	
				workData.xptClient->blockWorkSize = 0;
				float earnedShareValue = workData.xptClient->earnedShareValue;
				workData.xptClient->earnedShareValue = 0.0;
				primeStats.fShareValue += earnedShareValue;
				LeaveCriticalSection(&workData.xptClient->cs_workAccess);
				// print some block stats (of most recent received block data)
				double poolDiff = GetPrimeDifficulty( workData.workEntryQueue[workData.workEntrySize-1].nBitsShare);
				double blockDiff = GetPrimeDifficulty( workData.workEntryQueue[workData.workEntrySize-1].nBits);
				printf("---- New Block: %u - Diff: %.06f / %.06f\n", workData.workEntryQueue[workData.workEntrySize-1].height, blockDiff, poolDiff);
				printf("---- Total/Valid shares: [ %d / %d ]\n",valid_shares, total_shares);
				printf("---- 6CH count: %u - 7CH count: %u - Max diff: %.005f\n", primeStats.sixChainCount, primeStats.sevenChainCount, (float)primeStats.bestPrimeChainDifficultySinceLaunch); 
				printf("---- Server PrimorialMultiplier: %u\n", workData.workEntryQueue[workData.workEntrySize-1].fixedPrimorial);
				printf("---- Server PrimorialHashFactor: %u\n", workData.workEntryQueue[workData.workEntrySize-1].fixedHashFactor);
				printf("---- Allowed Sieverange: %u - %u\n", workData.workEntryQueue[workData.workEntrySize-1].sievesizeMin, workData.workEntryQueue[workData.workEntrySize-1].sievesizeMax);
				printf("---- Allowed Primetestrange: %u - %u\n", workData.workEntryQueue[workData.workEntrySize-1].primesToSieveMin, workData.workEntryQueue[workData.workEntrySize-1].primesToSieveMax);
				printf("---- Allowed Noncerange: %u - %u\n", workData.workEntryQueue[workData.workEntrySize-1].nonceMin, workData.workEntryQueue[workData.workEntrySize-1].nonceMax);
				printf("---- SieveChainLength: %u\n", workData.workEntryQueue[workData.workEntrySize-1].sieveChainLength);
				printf("---- New share value earned: %f\n", earnedShareValue);
				printf("\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\n");
				LeaveCriticalSection(&workData.cs);
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
	commandlineInput.mode = MINER_MODE_XPM_DEFAULT;
	//commandlineInput.sieveSize = 4000000; // default maxSieveSize
	//commandlineInput.sievePercentage = 35; // default 
	//commandlineInput.sievePrimeLimit = 0;

	//commandlineInput.sieveSize = 4000000; // default maxSieveSize
	commandlineInput.sieveSize = DEFAULT_SIEVE_SIZE; // default maxSieveSize
	commandlineInput.sievePrimeLimit = DEFAULT_PRIMES_TO_SIEVE; // default 
	// parse command lines
	jhMiner_parseCommandline(argc, argv);
	//// Sets max sieve size
	//uint32 nSievePercentage = commandlineInput.sievePercentage;
	//if (commandlineInput.sievePrimeLimit == 0) //default before parsing 
	//	commandlineInput.sievePrimeLimit = vPrimesSize;  //default is sieveSize 
	//if( nSievePercentage >= 100 )
	//	commandlineInput.sievePrimeLimit = (uint32)(((uint64)nSievePercentage*(uint64)vPrimesSize)/100ULL); // allow sieve prime percentages larger than 100%
	// set miner settings
	minerSettings.nPrimesToSieve = commandlineInput.sievePrimeLimit;
	minerSettings.nSieveSize = commandlineInput.sieveSize;
	minerSettings.sievePercentage = commandlineInput.sievePrimeLimit * 100 / commandlineInput.sieveSize;
	if( commandlineInput.host == NULL )
	{
		printf("Missing -o option\n");
		exit(-1);
	}
	printf("\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n");
	printf("\xBA  jhPrimeminer (v0.5 beta r4)                                  \xBA\n");
	printf("\xBA  contributors: hg5fm, x3maniac, jh                            \xBA\n");
	printf("\xBA  credits: Sunny King for the original Primecoin client&miner  \xBA\n");
	printf("\xBA  credits: mikaelh for the performance optimizations           \xBA\n");
	printf("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
	printf("Launching miner...\n");
	if( commandlineInput.mode == MINER_MODE_XPM_MULTIPASSSIEVE )
		printf("Notice:\n   This release contains an experimental algorithm that can cause repeated\n   share submissions. Dont worry if you receive a few rejected shares in a row.\n");
	mallocSpeedupInit();
	mallocSpeedupInitPerThread();
	// set priority lower so the user still can do other things
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	// init memory speedup (if not already done in preMain)
	//mallocSpeedupInit();
	if( pctx == NULL )
		pctx = BN_CTX_new();
	// init prime table
	GeneratePrimeTable(commandlineInput.sievePrimeLimit);
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
		exit(-1);
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

	jsonLocalPrimeCoin.ip = "127.0.0.1";
	jsonLocalPrimeCoin.port = 9912;
	jsonLocalPrimeCoin.authUser = "primecoinrpc";
	jsonLocalPrimeCoin.authPass = "x";


	// init stats
	primeStats.primeLastUpdate = GetTickCount();
	primeStats.shareFound = false;
	primeStats.shareRejected = false;
	primeStats.primeChainsFound = 0;
	primeStats.foundShareCount = 0;
	primeStats.fourChainCount = 0;
	primeStats.fiveChainCount = 0;
	primeStats.sixChainCount = 0;
	primeStats.sevenChainCount = 0;
	primeStats.fShareValue = 0;
	primeStats.cunningham1Count = 0;
	primeStats.cunningham2Count = 0;
	primeStats.cunninghamBiTwinCount = 0;

	// setup thread count and print info
	printf("Using %d threads\n", commandlineInput.numThreads);
	printf("Sieve size: %d\n", minerSettings.nSieveSize);
	printf("Primes: %d\n", minerSettings.nPrimesToSieve);
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
			// repeat connect & login until it is successful (with 45 seconds delay)
			while ( true )
			{
				workData.xptClient = xptClient_connect(&jsonRequestTarget, commandlineInput.numThreads);
				if( workData.xptClient != NULL )
					break;
				printf("Failed to connect, retry in 45 seconds\n");
				Sleep(1000*45);
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
			// try again in 45 seconds
			printf("x.pushthrough authentication sequence failed, retry in 45 seconds\n");
			Sleep(45*1000);
		}
	}

	printf("\nSPH = 'Share per Hour', PPS = 'Primes per Second', Val/h = 'Share Value per Hour'\n\n");

	// enter different mainloops depending on protocol mode
	if( workData.protocolMode == MINER_PROTOCOL_GETWORK )
		return jhMiner_main_getworkMode();
	else if( workData.protocolMode == MINER_PROTOCOL_XPUSHTHROUGH )
		return jhMiner_main_xptMode();

	return 0;
}
