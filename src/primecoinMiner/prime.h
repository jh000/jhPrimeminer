// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

#if defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_X64)
#define USE_ROTATE
#endif

//#include "main.h"
#include "mpirxx.h"

extern uint32* vPrimes;
extern uint32	vPrimesSize;
extern uint32* vTwoInverses;
extern uint32	vTwoInversesSize;

static const uint256 hashBlockHeaderLimit = (uint256(1) << 255);
static const CBigNum bnOne = 1;
static const CBigNum bnTwo = 2;
static const CBigNum bnConst8 = 8;
static const CBigNum bnPrimeMax = (bnOne << 2000) - 1;
static const CBigNum bnPrimeMin = (bnOne << 255);
static const mpz_class mpzOne = 1;
static const mpz_class mpzTwo = 2;
static const mpz_class mpzConst8 = 8;
static const mpz_class mpzPrimeMax = (mpzOne << 2000) - 1;
static const mpz_class mpzPrimeMin = (mpzOne << 255);


extern unsigned int nTargetInitialLength;
extern unsigned int nTargetMinLength;

// Generate small prime table
void GeneratePrimeTable(unsigned int nSieveSize);
// Get next prime number of p
//bool PrimeTableGetNextPrime(unsigned int* p);
bool PrimeTableGetNextPrime(unsigned int& p);
// Get previous prime number of p
bool PrimeTableGetPreviousPrime(unsigned int& p);

// Compute primorial number p#
void BNPrimorial(unsigned int p, CBigNum& bnPrimorial);
void Primorial(unsigned int p, mpz_class& mpzPrimorial);
unsigned int PrimorialFast(unsigned int p);
// Compute the first primorial number greater than or equal to bn
//void PrimorialAt(CBigNum& bn, CBigNum& bnPrimorial);
void PrimorialAt(mpz_class& bn, mpz_class& mpzPrimorial);

// Test probable prime chain for: bnPrimeChainOrigin
// fFermatTest
//   true - Use only Fermat tests
//   false - Use Fermat-Euler-Lagrange-Lifchitz tests
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
//bool ProbablePrimeChainTest(const CBigNum& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin);
bool ProbablePrimeChainTest(const mpz_class& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin, bool fullTest =false);
bool ProbablePrimeChainTestOrig(const mpz_class& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin, bool fullTest =false);


static const unsigned int nFractionalBits = 24;
static const unsigned int TARGET_FRACTIONAL_MASK = (1u<<nFractionalBits) - 1;
static const unsigned int TARGET_LENGTH_MASK = ~TARGET_FRACTIONAL_MASK;
static const uint64 nFractionalDifficultyMax = (1ull << (nFractionalBits + 32));
static const uint64 nFractionalDifficultyMin = (1ull << 32);
static const uint64 nFractionalDifficultyThreshold = (1ull << (8 + 32));
static const unsigned int nWorkTransitionRatio = 32;
unsigned int TargetGetLimit();
unsigned int TargetGetInitial();
unsigned int TargetGetLength(unsigned int nBits);
bool TargetSetLength(unsigned int nLength, unsigned int& nBits);
unsigned int TargetGetFractional(unsigned int nBits);
uint64 TargetGetFractionalDifficulty(unsigned int nBits);
bool TargetSetFractionalDifficulty(uint64 nFractionalDifficulty, unsigned int& nBits);
std::string TargetToString(unsigned int nBits);
unsigned int TargetFromInt(unsigned int nLength);
bool TargetGetMint(unsigned int nBits, uint64& nMint);
bool TargetGetNext(unsigned int nBits, int64 nInterval, int64 nTargetSpacing, int64 nActualSpacing, unsigned int& nBitsNext);

// Mine probable prime chain of form: n = h * p# +/- 1
//bool MineProbablePrimeChain(CBlock& block, CBigNum& bnFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit);

// Check prime proof-of-work
enum // prime chain type
{
	PRIME_CHAIN_CUNNINGHAM1 = 1u,
	PRIME_CHAIN_CUNNINGHAM2 = 2u,
	PRIME_CHAIN_BI_TWIN     = 3u,
};
// bool CheckPrimeProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnPrimeChainMultiplier, unsigned int& nChainType, unsigned int& nChainLength);

// prime target difficulty value for visualization
double GetPrimeDifficulty(unsigned int nBits);
// Estimate work transition target to longer prime chain
unsigned int EstimateWorkTransition(unsigned int nPrevWorkTransition, unsigned int nBits, unsigned int nChainLength);
// prime chain type and length value
std::string GetPrimeChainName(unsigned int nChainType, unsigned int nChainLength);

inline void mpz_set_uint256(mpz_t r, uint256& u)
{
    mpz_import(r, 32 / sizeof(unsigned long), -1, sizeof(unsigned long), -1, 0, &u);
}


// Number of primes to test with fast divisibility testing
static const unsigned int nFastDivPrimes = 60;

class CPrimalityTestParams
{
public:
	// GMP variables
	mpz_t mpzE;
	mpz_t mpzR;
	mpz_t mpzRplusOne;

	// GMP C++ variables
	mpz_class mpzOriginMinusOne;
	mpz_class mpzOriginPlusOne;
	mpz_class N;

	// Big divisors for fast div test
	std::vector<unsigned long> vFastDivisors;
	std::vector<unsigned int> vFastDivSeq;
	unsigned int nFastDivisorsSize;

	// Values specific to a round
	unsigned int nBits;
	unsigned int nPrimorialSeq;

	// This is currently always false when mining
	static const bool fFermatTest = false;

	// Results
	unsigned int nChainLengthCunningham1;
	unsigned int nChainLengthCunningham2;
	unsigned int nChainLengthBiTwin;

	CPrimalityTestParams(unsigned int nBits, unsigned int nPrimorialSeq)
	{
		this->nBits = nBits;
		this->nPrimorialSeq = nPrimorialSeq;
		nChainLengthCunningham1 = 0;
		nChainLengthCunningham2 = 0;
		nChainLengthBiTwin = 0;
		mpz_init(mpzE);
		mpz_init(mpzR);
		mpz_init(mpzRplusOne);
	}

	~CPrimalityTestParams()
	{
		mpz_clear(mpzE);
		mpz_clear(mpzR);
		mpz_clear(mpzRplusOne);
	}
};

bool ProbablePrimeChainTestFast(const mpz_class& mpzPrimeChainOrigin, CPrimalityTestParams& testParams, uint32 sieveFlags);

#define SIEVE_FLAG_C1_COMPOSITE	(1<<0) // sieve for +1 -> -1
#define SIEVE_FLAG_C2_COMPOSITE	(1<<1) // sieve for -1 -> +1
#define SIEVE_FLAG_BT_COMPOSITE	(1<<2) // combined

// miner3 (OpenCL)
void openCL_init();
void openCL_createContext();

#endif

