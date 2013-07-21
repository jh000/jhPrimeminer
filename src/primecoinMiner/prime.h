// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

//#include "main.h"

//static const unsigned int nMaxSieveSize = 1000000u;
//static const unsigned int nMaxSieveSize = 2000000u;
//static const unsigned int nMaxSieveSize = 500000u;
//static const unsigned int nMaxSieveSize = 1000000u;//800000u;
extern unsigned int nMaxSieveSize;
static const uint256 hashBlockHeaderLimit = (uint256(1) << 255);
static const CBigNum bnOne = 1;
static const CBigNum bnTwo = 2;
static const CBigNum bnConst8 = 8;
static const CBigNum bnPrimeMax = (bnOne << 2000) - 1;
static const CBigNum bnPrimeMin = (bnOne << 255);

extern unsigned int nTargetInitialLength;
extern unsigned int nTargetMinLength;

// Generate small prime table
void GeneratePrimeTable();
// Get next prime number of p
bool PrimeTableGetNextPrime(unsigned int* p);
// Get previous prime number of p
bool PrimeTableGetPreviousPrime(unsigned int* p);

// Compute primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial);
// Compute the first primorial number greater than or equal to bn
void PrimorialAt(CBigNum& bn, CBigNum& bnPrimorial);

// Test probable prime chain for: bnPrimeChainOrigin
// fFermatTest
//   true - Use only Fermat tests
//   false - Use Fermat-Euler-Lagrange-Lifchitz tests
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
bool ProbablePrimeChainTest(const CBigNum& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin);

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

// Sieve of Eratosthenes for proof-of-work mining
class CSieveOfEratosthenes
{
	unsigned int nSieveSize; // size of the sieve
	unsigned int nBits; // target of the prime chain to search for
	uint256 hashBlockHeader; // block header hash
	CBigNum bnFixedFactor; // fixed factor to derive the chain

	// bitmaps of the sieve, index represents the variable part of multiplier
	//std::vector<bool> vfCompositeCunningham1;
	//std::vector<bool> vfCompositeCunningham2;
	//std::vector<bool> vfCompositeBiTwin;

	CAutoBN_CTX pctx;
	BIGNUM bn_constTwo;
	uint32 bignumData_constTwo[0x200/4];
	//BIGNUM bn_nDelta1;
	//uint32 bignumData_constTwo[0x200/4];

	unsigned int nPrimeSeq; // prime sequence number currently being processed
	unsigned int nCandidateMultiplier; // current candidate for power test


public:
	//bool* vfCompositeCunningham1;
	//bool* vfCompositeCunningham2;
	//bool* vfCompositeBiTwin;

	uint8* vfCompositeCunningham1;
	uint8* vfCompositeCunningham2;
	uint8* vfCompositeBiTwin;

	CSieveOfEratosthenes(unsigned int nSieveSize, unsigned int nBits, uint256 hashBlockHeader, CBigNum& bnFixedMultiplier)
	{
		this->nSieveSize = nSieveSize;
		this->nBits = nBits;
		this->hashBlockHeader = hashBlockHeader;
		this->bnFixedFactor = bnFixedMultiplier * CBigNum(hashBlockHeader);
		nPrimeSeq = 0;
		uint32 maskBytes = (nMaxSieveSize+7)/8;
		vfCompositeCunningham1 = (uint8*)malloc(sizeof(uint8)*maskBytes);
		RtlZeroMemory(vfCompositeCunningham1, sizeof(uint8)*maskBytes);
		vfCompositeCunningham2 = (uint8*)malloc(sizeof(uint8)*maskBytes);
		RtlZeroMemory(vfCompositeCunningham2, sizeof(uint8)*maskBytes);
		vfCompositeBiTwin = (uint8*)malloc(sizeof(uint8)*maskBytes);
		RtlZeroMemory(vfCompositeBiTwin, sizeof(uint8)*maskBytes);
		nCandidateMultiplier = 0;
		// init bn_constTwo
		bn_constTwo.d = (BN_ULONG*)bignumData_constTwo;
		bn_constTwo.dmax = 0x200/4;
		bn_constTwo.flags = BN_FLG_STATIC_DATA;
		bn_constTwo.neg = 0; 
		bn_constTwo.top = 1; 
		BN_set_word(&bn_constTwo, 2);
	}



	~CSieveOfEratosthenes()
	{
		free(vfCompositeCunningham1);
		free(vfCompositeCunningham2);
		free(vfCompositeBiTwin);
	}

	// Get total number of candidates for power test
	unsigned int GetCandidateCount()
	{
		unsigned int nCandidates = 0;
		for (unsigned int nMultiplier = 0; nMultiplier < nSieveSize; nMultiplier++)
		{
			uint32 byteIdx = nMultiplier>>3;
			uint32 mask = 1<<(nMultiplier&7);
			if (!(vfCompositeCunningham1[byteIdx]&mask) ||
				!(vfCompositeCunningham2[byteIdx]&mask) ||
				!(vfCompositeBiTwin[byteIdx]&mask))
				nCandidates++;
		}
		return nCandidates;
	}

	// Scan for the next candidate multiplier (variable part)
	// Return values:
	//   True - found next candidate; nVariableMultiplier has the candidate
	//   False - scan complete, no more candidate and reset scan
	bool GetNextCandidateMultiplier(unsigned int& nVariableMultiplier)
	{
		for(;;)
		{
			nCandidateMultiplier++;
			if (nCandidateMultiplier >= nSieveSize)
			{
				nCandidateMultiplier = 0;
				return false;
			}
			uint32 byteIdx = nCandidateMultiplier>>3;
			uint32 mask = 1<<(nCandidateMultiplier&7);

			if (!(vfCompositeCunningham1[byteIdx]&mask) ||
				!(vfCompositeCunningham2[byteIdx]&mask) ||
				!(vfCompositeBiTwin[byteIdx]&mask))
			{
				nVariableMultiplier = nCandidateMultiplier;
				return true;
			}
		}
	}

	// Weave the sieve for the next prime in table
	// Return values:
	//   True  - weaved another prime; nComposite - number of composites removed
	//   False - sieve already completed
	bool WeaveOriginal();
	//bool WeaveFast();
	//bool WeaveFast2();
	bool WeaveFastAll();
	// bool WeaveAlt();
};

#endif
