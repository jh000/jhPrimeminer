#include"global.h"

#include<intrin.h>

primeStats_t primeStats = {0};
total_shares = 0;
valid_shares = 0;
unsigned int nMaxSieveSize;

bool error(const char *format, ...)
{
	puts(format);
	//__debugbreak();
	return false;
}

//customBuffer_t* list_primeTable; // alias vPrimes
//uint32* vPrimes;
//uint32 vPrimesSize;
//
////static const unsigned int nMaxSieveSize = 1000000u;
////static const uint256 hashBlockHeaderLimit = (uint256(1) << 255);
////static const CBigNum bnOne = 1;
////static const CBigNum bnPrimeMax = (bnOne << 2000) - 1;
////static const CBigNum bnPrimeMin = (bnOne << 255);
//
//void GeneratePrimeTable()
//{
//	static const unsigned int nPrimeTableLimit = nMaxSieveSize;
//	//vPrimes.clear();
//	vPrimesSize = 0;
//	vPrimes = (uint32*)malloc(sizeof(uint32)*nPrimeTableLimit);
//	// Generate prime table using sieve of Eratosthenes
//	//std::vector<bool> vfComposite (nPrimeTableLimit, false);
//	uint8* vfComposite = (uint8*)malloc(nPrimeTableLimit);
//	RtlZeroMemory(vfComposite, nPrimeTableLimit);
//	for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
//	{
//		if (vfComposite[nFactor])
//			continue;
//		for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
//			vfComposite[nComposite] = true;
//	}
//	for (unsigned int n = 2; n < nPrimeTableLimit; n++)
//		if (!vfComposite[n])
//		{
//			vPrimes[vPrimesSize] = n;
//			vPrimesSize++;
//			//customBuffer_add(list_primeTable, &n);//vPrimes.push_back(n);
//		}
//	printf("GeneratePrimeTable() : prime table [1, %d] generated with %lu primes", nPrimeTableLimit, vPrimesSize);
//	//BOOST_FOREACH(unsigned int nPrime, vPrimes)
//	//    printf(" %u", nPrime);
//	printf("\n");
//	free(vfComposite);
//	vPrimes = (uint32*)realloc(vPrimes, sizeof(uint32)*vPrimesSize);
//}
//
//// Compute Primorial number p#
//void Primorial(unsigned int p, BIGNUM* bnPrimorial)
//{
//	__debugbreak();
//	//Bignum_SetUINT32(bnPrimorial, 1);
//	////bnPrimorial = 1;
//	////BOOST_FOREACH(unsigned int nPrime, vPrimes)
//	//BIGNUM bnTemp;
//	//for(uint32 i=0; i<vPrimesSize; i++)
//	//{
//	//	if (vPrimes[i] > p)
//	//		break;
//	//	
//	//	Bignum_SetUINT32(&bnTemp, vPrimes[i]);
//	//	Bignum_Mul(bnPrimorial, bnPrimorial, &bnTemp);
//	//	//bnPrimorial *= nPrime;
//
//	//}
//}
//
//// Get next prime number of p
//bool PrimeTableGetNextPrime(unsigned int* p)
//{
//	for(uint32 i=0; i<vPrimesSize; i++)
//	{
//		if (vPrimes[i] > *p)
//		{
//			*p = vPrimes[i];
//			return true;
//		}
//	}
//	return false;
//}
//
//// Get previous prime number of p
//bool PrimeTableGetPreviousPrime(unsigned int* p)
//{
//	uint32 nPrevPrime = 0;
//	for(uint32 i=0; i<vPrimesSize; i++)
//	{
//		if (vPrimes[i] >= *p)
//			break;
//		nPrevPrime = vPrimes[i];
//	}
//	if (nPrevPrime)
//	{
//		*p = nPrevPrime;
//		return true;
//	}
//	return false;
//}


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

	// swap endianness for SHA256
	//for(uint32 i=0; i<80/4; i++)
	//{
	//	*(uint32*)(blockHashDataInput+i*4) = _swapEndianessU32(*(uint32*)(blockHashDataInput+i*4));
	//}

	//for(uint32 i=1; i<64/4; i++)
	//{
	//	*(uint32*)(blockHashDataInput+i*4) = _swapEndianessU32(*(uint32*)(blockHashDataInput+i*4));
	//}

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
	//sint32 lengthBN = primecoinBlock->bnPrimeChainMultiplier.dmax;
	//
	////memcpy(blockHashDataInput+80+4, primecoinBlock->bnPrimeChainMultiplier.d, lengthBN*4);
	//for(uint32 i=0; i<lengthBN*4; i++)
	//{
	//	blockHashDataInput[80+4+i] = *((uint8*)primecoinBlock->bnPrimeChainMultiplier.d + (lengthBN*4-i-1));	
	//}

	uint32 writeIndex = 80;

	sint32 lengthBN = 0;
	std::vector<unsigned char> bnSerializeData = primecoinBlock->bnPrimeChainMultiplier.getvch();
	

	lengthBN = bnSerializeData.size();
	//*(uint32*)(blockHashDataInput+80) = _swapEndianessU32(lengthBN); // this is always stored as big endian
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


//bool CheckPrimeProofOfWork(BIGNUM* hashBlockHeader, unsigned int nBits, BIGNUM* bnPrimeChainMultiplier, unsigned int* nChainType, unsigned int* nChainLength);

//#define ALLOC_SPEED_UP_BUFFERS		(32*5)
//#define ALLOC_SPEED_UP_BUFFER_SIZE	(4096) // 4kb	
//
//__declspec( thread ) uint8* allocSpeedupBuffer;
//__declspec( thread ) uint8* allocSpeedupBufferEnd;
//__declspec( thread ) uint32 allocSpeedupMask[ALLOC_SPEED_UP_BUFFERS/32] = {0}; // bit not set -> free, bit set -> used
////__declspec( thread ) uint32 allocSpeedupBufferLog[ALLOC_SPEED_UP_BUFFERS];
////__declspec( thread ) uint32 allocSpeedupBufferLogTime[ALLOC_SPEED_UP_BUFFERS];
//
//uint32 inline _getFirstBitIndex(uint32 mask)
//{
//	unsigned long index;
//	_BitScanForward(&index, mask);
//	return index;
//}
//
///*
// * The mallocEx/reallocEx/freeEx code below was used to speed up memory allocation for bignums (or other OpenSSL CRYPTO stuff) by a factor of 10x
// * It worked quite well, but due to a memory leak of unknown origin I had to disable it.
// * The miner seems to occasionally allocate a bignum but never free it again, thus filling the limited 'allocSpeedupBuffer' until it is full forcing the mallocEx method to use standard malloc()
// */
//void* mallocEx(size_t size,const char * file,int lineNumber)
//{
//	if( size <= ALLOC_SPEED_UP_BUFFER_SIZE)
//	{
//		// do a stack walk (debugging)
//		//// stack walk
//		//uint32* stackOffset = ((uint32*)_AddressOfReturnAddress())-1;
//		//uint32 returnAddress = stackOffset[1];
//		//uint32 epbBaseAddress = stackOffset[0];
//		//uint32 epbBaseAddressOrg = epbBaseAddress;
//		//uint32 stackHash = returnAddress;
//		//sint32 stackDif = (sint32)((uint32)stackOffset - epbBaseAddress);
//		//if( stackDif < 0 ) stackDif = -stackDif;
//		//if( stackDif <= 1024*800 )
//		//{
//		//	for(sint32 c=0; c<12; c++)
//		//	{
//		//		stackDif = (sint32)((uint32)stackOffset - epbBaseAddress);
//		//		if( stackDif < 0 ) stackDif = -stackDif;
//		//		if( stackDif >= 1024*800 )
//		//			break;
//		//		stackOffset = (uint32*)(epbBaseAddress);
//		//		returnAddress = stackOffset[1];
//		//		epbBaseAddress = stackOffset[0];
//		//		if( returnAddress == NULL || epbBaseAddress == NULL )
//		//			break;
//		//		stackHash += returnAddress;
//		//	}
//		//}
//		//// end of stack walk
//		for(uint32 i=0; i<(ALLOC_SPEED_UP_BUFFERS/32); i++)
//		{
//			if( allocSpeedupMask[i] != 0xFFFFFFFF )
//			{
//				uint32 bufferIndex = _getFirstBitIndex(~allocSpeedupMask[i]);
//				if( allocSpeedupMask[i] & (1<<bufferIndex) )
//					__debugbreak();
//				allocSpeedupMask[i] |= (1<<bufferIndex);
//				//allocSpeedupBufferLog[i*32+bufferIndex] = stackHash;
//				//allocSpeedupBufferLogTime[i*32+bufferIndex] = GetTickCount();
//
//				return allocSpeedupBuffer + (i*32+bufferIndex)*ALLOC_SPEED_UP_BUFFER_SIZE;
//			}
//		}
//		//printf("All used :( (Stack Hash: %08X)\n", stackHash);
//	}
//	return malloc(size);
//}
//
//void freeEx(void* buffer)
//{
//	if( buffer >= allocSpeedupBuffer && buffer < allocSpeedupBufferEnd )
//	{
//		uint32 bufferIndex = ((uint8*)buffer - allocSpeedupBuffer)/ALLOC_SPEED_UP_BUFFER_SIZE;
//		allocSpeedupMask[bufferIndex/32] &= ~(1<<(bufferIndex&31));
//		return;
//	}
//	//printf("Free: %08x\n", buffer);
//	free(buffer);
//}
//
//void* reallocEx(void* buffer, size_t newSize,const char * file,int lineNumber)
//{
//	if( buffer >= allocSpeedupBuffer && buffer < allocSpeedupBufferEnd )
//		__debugbreak(); // realloc not supported, but doesnt seem to be used anyways
//	return realloc(buffer, newSize);
//}
//
//void mallocSpeedupInit()
//{
//	if( allocSpeedupBuffer )
//		return;
//	allocSpeedupBuffer = (uint8*)malloc(ALLOC_SPEED_UP_BUFFERS*ALLOC_SPEED_UP_BUFFER_SIZE);
//	allocSpeedupBufferEnd = allocSpeedupBuffer + (ALLOC_SPEED_UP_BUFFERS*ALLOC_SPEED_UP_BUFFER_SIZE);
//}

//
//int main()
//{
//	allocSpeedupBuffer = (uint8*)malloc(ALLOC_SPEED_UP_BUFFERS*ALLOC_SPEED_UP_BUFFER_SIZE);
//	allocSpeedupBufferEnd = allocSpeedupBuffer + (ALLOC_SPEED_UP_BUFFERS*ALLOC_SPEED_UP_BUFFER_SIZE);
//	CRYPTO_set_mem_functions(mallocEx, reallocEx, freeEx);
//	// init stuff
//	//list_primeTable = customBuffer_create(128, sizeof(uint32));
//	//vPrimesSize = 0;
//	// generate primetable
//	//GeneratePrimeTable();
//	// todo
//	GeneratePrimeTable();
//	//__debugbreak();
//
//	// initial test block (block with index 100)
//	primecoinBlock_t primecoinBlock = {0};
//	primecoinBlock.version = 2;
//	yPoolWorkMgr_parseHexStringLE("9e561a9eb64b3cbb15679075220f72597b49427f18694976dcabaabc84e0b3bb", 32*2, primecoinBlock.prevBlockHash);
//	yPoolWorkMgr_parseHexStringLE("68b45436f76b7290bde0be84c3c1b695c3364ae44600b31e0cf86f5417bed09d", 32*2, primecoinBlock.merkleRoot);
//	primecoinBlock.timestamp = 1373227362;
//	primecoinBlock.nBits = 0x070089c3; // 8 bit int, 24 bit fract
//	//0x70089c3;
//	primecoinBlock.nonce = 159;
//
//	//uint8 blockHeaderHash[32];
//	primecoinBlock_generateHeaderHash(&primecoinBlock, primecoinBlock.blockHeaderHash.begin());
//
//	char* primeOriginString = "5366987322649438967363624901152502571853327872155536768049073037591356928419358636181903971125351980";
//	//uint256 primeCoinOriginData[512];
//	//yPoolWorkMgr_parseHexString(primeOriginString, fStrLen(primeOriginString), primeCoinOriginData);
//	//CBigNum(primeCoinOriginData);
//	
//
//
//	primecoinBlock.bnPrimeChainMultiplier = CBigNum(primeOriginString) / CBigNum(primecoinBlock.blockHeaderHash);
//	CBigNum lolTest = CBigNum(primeOriginString) % CBigNum(primecoinBlock.blockHeaderHash);
//	puts(lolTest.ToString().c_str());
//	
//
//
//	//bn_
//	//important things: primeOriginString is NOT HEX...
//
//	uint8 blockHashFinal[32];
//	primecoinBlock_generateBlockHash(&primecoinBlock, blockHashFinal);
//
//	puts("");
//	for(uint32 i=0; i<128; i++)
//	{
//		if( (i&15)==0 )
//			puts("");
//		printf("%02x", *((uint8*)&primecoinBlock+i));
//	}
//
//	BitcoinMiner(&primecoinBlock);
//	//BIGNUM bn_blockHeaderHash;
//	////Bignum_Read_BigEndian(&bn_blockHeaderHash, blockHeaderHash, 32);
//	//Bignum_Read(&bn_blockHeaderHash, blockHeaderHash, 32);
//
//
//	//char* primeOriginString = "5366987322649438967363624901152502571853327872155536768049073037591356928419358636181903971125351980";
//	//uint8 primeCoinOriginData[512];
//	//yPoolWorkMgr_parseHexString(primeOriginString, fStrLen(primeOriginString), primeCoinOriginData);
//	//BIGNUM bnPrimeChainOrigin;
//	//Bignum_Read(&bnPrimeChainOrigin, primeCoinOriginData, fStrLen(primeOriginString)/2);
//	//Bignum_Div(&primecoinBlock.bnPrimeChainMultiplier, &bnPrimeChainOrigin, &bn_blockHeaderHash);
//
//	//// uint32 
//
//	//// bnPrimeChainMultiplier
//
//	//// start mining
//	//static const unsigned int nPrimorialHashFactor = 7;
//	//unsigned int nPrimorialMultiplier = nPrimorialHashFactor;
//	//
//	//while( true )
//	//{
//	//	BIGNUM bn_hashFactor;
//	//	Primorial(nPrimorialHashFactor, &bn_hashFactor);
//
//	//	BIGNUM bn_temp1;
//	//	// bn_blockHeaderHash
//
//	//	//while ((pblock->GetHeaderHash() < hashBlockHeaderLimit || CBigNum(pblock->GetHeaderHash()) % bnHashFactor != 0) && pblock->nNonce < 0xffff0000)
//	//	//	pblock->nNonce++;
//	//	//if (pblock->nNonce >= 0xffff0000)
//	//	//	continue; <--- This will cause the extra nonce to increase (alias calculating a new merkle root)
//
//
//	//	Bignum_Mod(&bn_temp1, &bn_blockHeaderHash, &bn_hashFactor);
//
//	//	Bignum_Print(&bn_blockHeaderHash);
//	//	Bignum_Print(&bn_temp1);
//	//	// pblock->GetHeaderHash() < hashBlockHeaderLimit <--- This makes sure the highest bit IS set?
//
//	//	// CheckProofOfWork(pblock->GetHeaderHash(), pblock->nBits, pblock->bnPrimeChainMultiplier, pblock->nPrimeChainType, pblock->nPrimeChainLength))
//
//	//	// "primechain" : "1CC07.04d7db",
//	//	// the two values below dont have to be set, they are set by the call to CheckPrimeProofOfWork()?
//	//	unsigned int nPrimeChainType = PRIME_CHAIN_CUNNINGHAM1;
//	//	unsigned int nPrimeChainLength = 0x0704d7db;
//
//
//	//	
//	//	bool resultX = CheckPrimeProofOfWork(&bn_blockHeaderHash, _swapEndianessU32(primecoinBlock.bits), &primecoinBlock.bnPrimeChainMultiplier, &nPrimeChainType, &nPrimeChainLength);
//	//	__debugbreak();
//
//	//	for(uint32 i=0; i<0x10000; i++)
//	//	{
//	//		
//	//	}
//	//}
//	
//
//	return 0;
//}


typedef struct  
{
	CRITICAL_SECTION cs;
	bool dataIsValid;
	// xpm
	uint8 data[128];
	uint32 dataHash; // used to detect work data changes
	uint8 serverData[32]; // contains data from the server 
}workData_t;

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
		// rpc call worked, sooooo.. is the bitcoin client happy with the result?
		jsonObject_t* jsonReturnValueBool = jsonObject_getParameter(jsonReturnValue, "result");
		if( jsonObject_isTrue(jsonReturnValueBool) )
		{
			total_shares++;
			valid_shares++;
			printf("Valid share found!");
			printf("[ %d / %d ]", valid_shares, total_shares);
			jsonObject_freeObject(jsonReturnValue);
			return true;
		}
		else
		{
			total_share++;
			// the server says no to this share :(
			printf("Server rejected share (BlockHeight: %d/%d nBits: 0x%08X)\n", primecoinBlock->serverData.blockHeight, jhMiner_getCurrentWorkBlockHeight(), primecoinBlock->serverData.client_shareBits);
			jsonObject_freeObject(jsonReturnValue);
			return false;
		}
	}
	jsonObject_freeObject(jsonReturnValue);
	return false;
}

void jhMiner_queryWork_primecoin()
{
	sint32 rpcErrorCode = 0;
	jsonObject_t* jsonReturnValue = jsonClient_request(&jsonRequestTarget, "getwork", NULL, &rpcErrorCode);
	if( jsonReturnValue == NULL )
	{
		printf("Getwork() failed with %serror code %d\n", (rpcErrorCode>1000)?"http ":"", rpcErrorCode>1000?rpcErrorCode-1000:rpcErrorCode);
		workData.dataIsValid = false;
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
			workData.dataIsValid = false;
			jsonObject_freeObject(jsonReturnValue);
			return;
		}
		//printf("Success :)\nHere are some values from getwork:\n");
		// data
		uint32 stringData_length = 0;
		uint8* stringData_data = jsonObject_getStringData(jsonResult_data, &stringData_length);
		//printf("data: %.*s...\n", (sint32)min(48, stringData_length), stringData_data);

		EnterCriticalSection(&workData.cs);
		yPoolWorkMgr_parseHexString((char*)stringData_data, min(128*2, stringData_length), workData.data);
		workData.dataIsValid = true;
		// get server data
		uint32 stringServerData_length = 0;
		uint8* stringServerData_data = jsonObject_getStringData(jsonResult_serverData, &stringServerData_length);
		RtlZeroMemory(workData.serverData, 32);
		if( jsonResult_serverData )
			yPoolWorkMgr_parseHexString((char*)stringServerData_data, min(128*2, 32*2), workData.serverData);
		// generate work hash
		uint32 workDataHash = 0x5B7C8AF4;
		for(uint32 i=0; i<stringData_length/2; i++)
		{
			workDataHash = (workDataHash>>29)|(workDataHash<<3);
			workDataHash += (uint32)workData.data[i];
		}
		workData.dataHash = workDataHash;
		LeaveCriticalSection(&workData.cs);
		jsonObject_freeObject(jsonReturnValue);
	}
}

/*
 * Returns the hash of the current work data (32bit block data hash)
 */
uint32 jhMiner_getCurrentWorkHash()
{
	return workData.dataHash;
}

/*
 * Returns the block height of the most recently received workload
 */
uint32 jhMiner_getCurrentWorkBlockHeight()
{
	return ((serverData_t*)workData.serverData)->blockHeight;
}

int jhMiner_workerThread(int threadIndex)
{
	//mallocSpeedupInit();
	while( true )
	{
		uint8 localBlockData[128];
		// copy block data from global workData
		uint32 workDataHash = 0;
		uint8 serverData[32];
		while( workData.dataIsValid == false ) Sleep(200);
		EnterCriticalSection(&workData.cs);
		memcpy(localBlockData, workData.data, 128);
		//seed = workData.seed;
		memcpy(serverData, workData.serverData, 32);
		workDataHash = workData.dataHash;
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
		// ypool uses a special encrypted serverData value to speedup identification of merkleroot and share data
		memcpy(&primecoinBlock.serverData, serverData, 32);
		// data hash to check for updated work data
		primecoinBlock.workDataHash = workDataHash;
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
		//sets default maxSieveSize
		commandlineInput.sieveSize = 1000000;
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

int main(int argc, char **argv)
{
	// setup some default values
	commandlineInput.port = 8332;
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	commandlineInput.numThreads = sysinfo.dwNumberOfProcessors;
	commandlineInput.numThreads = max(commandlineInput.numThreads, 1);
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
	printf("\xBA  jhPrimeMiner (v0.21 beta)                                    \xBA\n");
	printf("\xBA  author: JH (http://ypool.net)                                \xBA\n");
	printf("\xBA  Credits: Sunny King for the original Primecoin client&miner  \xBA\n");
	printf("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
	printf("Launching miner...\n");
	// set priority lower so the user still can do other things
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	// init memory speedup (if not already done in preMain)
	//mallocSpeedupInit();
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
//#ifdef _WIN64
//	if( ipListPtr[0] )
//	{
//		ip = (uint32)*(uint8*)((uint8*)ipListPtr[0]+0);
//		ip |= ((uint32)*(uint8*)((uint8*)ipListPtr[0]+1))<<8;
//		ip |= ((uint32)*(uint8*)((uint8*)ipListPtr[0]+2))<<16;
//		ip |= ((uint32)*(uint8*)((uint8*)ipListPtr[0]+3))<<24;
//	}
//#else
//	if( ipListPtr[0] )
//	{
//		ip = *(uint32*)ipListPtr[0];
//	}
//#endif
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
	sint32 nThreads = commandlineInput.numThreads;
	printf("Using %d threads\n", nThreads);
	printf("Username: %s\n", jsonRequestTarget.authUser);
	printf("Password: %s\n", jsonRequestTarget.authPass);
	// initial query new work
	jhMiner_queryWork_primecoin();
	// start threads
	for(sint32 threadIdx=0; threadIdx<nThreads; threadIdx++)
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)jhMiner_workerThread, (LPVOID)threadIdx, 0, 0);
	// main thread, query work every 8 seconds
	sint32 loopCounter = 0;
	while( true )
	{
		// query new work
		jhMiner_queryWork_primecoin();
		// calculate stats every second tick
		if( loopCounter&1 )
		{
			double statsPassedTime = (double)GetTickCount() - primeStats.primeLastUpdate;
			if( statsPassedTime < 1.0 )
				statsPassedTime = 1.0; // avoid division by zero
			double primesPerSecond = (double)primeStats.primeChainsFound / (statsPassedTime / 1000.0);
			primeStats.primeLastUpdate = GetTickCount();
			primeStats.primeChainsFound = 0;
			uint32 bestDifficulty = primeStats.bestPrimeChainDifficulty;
			primeStats.bestPrimeChainDifficulty = 0;
			double primeDifficulty = (double)bestDifficulty / (double)0x1000000;
			if( workData.dataIsValid )
				printf("primes/s: %d best difficulty: %f\n", (sint32)primesPerSecond, (float)primeDifficulty);
		}		
		// wait and check some stats
		uint32 time_updateWork = GetTickCount() + 4000;
		while( true )
		{
			if( GetTickCount() >= time_updateWork )
				break;
			Sleep(500);
		}
		loopCounter++;
	}
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
