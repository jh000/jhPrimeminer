#pragma comment(lib,"Ws2_32.lib")
#include<Winsock2.h>
#include<ws2tcpip.h>
#include"jhlib/JHLib.h"

#include<stdio.h>

#include"sha256.h"
#include"ripemd160.h"
//#include"bignum_custom.h"
static const int PROTOCOL_VERSION = 70001;

#include<openssl/bn.h>
#include"uint256.h"
#include"bignum2.h"
//#include"bignum_custom.h"

#include"prime.h"
#include"jsonrpc.h"

static const int64 COIN = 100000000;
static const int64 CENT = 1000000;


typedef struct  
{
	/* +0x00 */ uint32 seed;
	/* +0x04 */ uint32 nBitsForShare;
	/* +0x08 */ uint32 blockHeight;
	/* +0x0C */ uint32 padding1;
	/* +0x10 */ uint32 padding2;
	/* +0x14 */ uint32 client_shareBits; // difficulty score of found share (the client is allowed to modify this value, but not the others)
	/* +0x18 */ uint32 serverStuff1;
	/* +0x1C */ uint32 serverStuff2;
}serverData_t;

typedef struct  
{
	volatile uint32 primeChainsFound;
	// since we can generate many (useless) primes ultra fast if we simply set sieve size low, 
	// its better if we only count primes with at least a given difficulty
	//volatile uint32 qualityPrimesFound;
	volatile uint32 bestPrimeChainDifficulty;
	uint32 primeLastUpdate;
}primeStats_t;

extern primeStats_t primeStats;

typedef struct  
{
	uint32	version;
	uint8	prevBlockHash[32];
	uint8	merkleRoot[32];
	uint32	timestamp;
	uint32	nBits;
	uint32	nonce;
	// GetHeaderHash() goes up to this offset (4+32+32+4+4+4=80 bytes)
	uint256 blockHeaderHash;
	CBigNum bnPrimeChainMultiplier;
	// other
	uint32 workDataHash;
	serverData_t serverData;
}primecoinBlock_t;

extern jsonRequestTarget_t jsonRequestTarget; // rpc login data

// prototypes from main.cpp
bool error(const char *format, ...);
bool jhMiner_pushShare_primecoin(uint8 data[256], primecoinBlock_t* primecoinBlock);
void primecoinBlock_generateHeaderHash(primecoinBlock_t* primecoinBlock, uint8 hashOutput[32]);
uint32 _swapEndianessU32(uint32 v);
uint32 jhMiner_getCurrentWorkHash();
uint32 jhMiner_getCurrentWorkBlockHeight();

void BitcoinMiner(primecoinBlock_t* primecoinBlock, sint32 threadIndex);
