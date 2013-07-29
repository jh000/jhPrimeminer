// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

//#include "main.h"

static const uint256 hashBlockHeaderLimit = (uint256(1) << 255);
static const mpz_class bnOne = 1;
static const mpz_class bnTwo = 2;
static const mpz_class bnConst8 = 8;
static const mpz_class bnPrimeMax = (bnOne << 2000) - 1;
static const mpz_class bnPrimeMin = (bnOne << 255);

extern unsigned int nTargetInitialLength;
extern unsigned int nTargetMinLength;

// Generate small prime table
void GeneratePrimeTable();
// Get next prime number of p
bool PrimeTableGetNextPrime(unsigned int* p);
// Get previous prime number of p
bool PrimeTableGetPreviousPrime(unsigned int* p);

// Compute primorial number p#
void Primorial(unsigned int p, mpz_class& bnPrimorial);
// Compute the first primorial number greater than or equal to bn
void PrimorialAt(mpz_class& bn, mpz_class& bnPrimorial);

// Test probable prime chain for: bnPrimeChainOrigin
// fFermatTest
//   true - Use only Fermat tests
//   false - Use Fermat-Euler-Lagrange-Lifchitz tests
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
bool ProbablePrimeChainTest(const mpz_class& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin);

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
//bool MineProbablePrimeChain(CBlock& block, mpz_class& bnFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit);

// Check prime proof-of-work
enum // prime chain type
{
	PRIME_CHAIN_CUNNINGHAM1 = 1u,
	PRIME_CHAIN_CUNNINGHAM2 = 2u,
	PRIME_CHAIN_BI_TWIN     = 3u,
};
// bool CheckPrimeProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const mpz_class& bnPrimeChainMultiplier, unsigned int& nChainType, unsigned int& nChainLength);

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
// Sieve of Eratosthenes for proof-of-work mining
class CSieveOfEratosthenes
{
	unsigned int nSieveSize; // size of the sieve
	unsigned int nBits; // target of the prime chain to search for
	uint256 hashBlockHeader; // block header hash
	CBigNum bnFixedFactor; // fixed factor to derive the chain
	// bitmaps of the sieve, index represents the variable part of multiplier
	CAutoBN_CTX pctx;
	BIGNUM bn_constTwo;
	uint32 bignumData_constTwo[0x200/4];
	unsigned int nPrimeSeq; // prime sequence number currently being processed
	unsigned int nCandidateMultiplier; // current candidate for power test

public:
	//bool* vfCompositeCunningham1;
	//bool* vfCompositeCunningham2;
	//bool* vfCompositeBiTwin;

	uint32* vfCompositeCunningham1;
	uint32* vfCompositeCunningham2;
	uint32* vfCompositeBiTwin;

	CSieveOfEratosthenes(unsigned int nSieveSize, unsigned int nBits, uint256 hashBlockHeader, mpz_class& bnFixedMultiplier)
	{
		this->nSieveSize = nSieveSize;
		this->nBits = nBits;
		this->hashBlockHeader = hashBlockHeader;
		CBigNum bnFixedMultiplierTemp;
		bnFixedMultiplierTemp.SetHex(bnFixedMultiplier.get_str(16));
		this->bnFixedFactor = bnFixedMultiplierTemp * CBigNum(hashBlockHeader);
		nPrimeSeq = 0;
		uint32 maskBytes = (commandlineInput.sieveSize+31)/32;
		vfCompositeCunningham1 = (uint32*)malloc(sizeof(uint32)*maskBytes);
		RtlZeroMemory(vfCompositeCunningham1, sizeof(uint32)*maskBytes);
		vfCompositeCunningham2 = (uint32*)malloc(sizeof(uint32)*maskBytes);
		RtlZeroMemory(vfCompositeCunningham2, sizeof(uint32)*maskBytes);
		if( commandlineInput.sieveForTwinChains )
		{
			vfCompositeBiTwin = (uint32*)malloc(sizeof(uint32)*maskBytes);
			RtlZeroMemory(vfCompositeBiTwin, sizeof(uint32)*maskBytes);
		}
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
		if( commandlineInput.sieveForTwinChains )
			free(vfCompositeBiTwin);
	}

	// Get total number of candidates for power test
	unsigned int GetCandidateCount()
	{
		unsigned int nCandidates = 0;
		if( commandlineInput.sieveForTwinChains )
		{
			for (unsigned int nMultiplier = 0; nMultiplier < nSieveSize; nMultiplier++)
			{
				uint32 byteIdx = nMultiplier>>5;
				uint32 mask = 1<<(nMultiplier&31);
				if (!(vfCompositeCunningham1[byteIdx]&mask) ||
					!(vfCompositeCunningham2[byteIdx]&mask) ||
					!(vfCompositeBiTwin[byteIdx]&mask))
					nCandidates++;
			}
			return nCandidates;
		}
		else
		{
			for (unsigned int nMultiplier = 0; nMultiplier < nSieveSize; nMultiplier++)
			{
				uint32 byteIdx = nMultiplier>>5;
				uint32 mask = 1<<(nMultiplier&31);
				if (!(vfCompositeCunningham1[byteIdx]&mask) ||
					!(vfCompositeCunningham2[byteIdx]&mask) )
					nCandidates++;
			}
			return nCandidates;
		}
	}

	// Scan for the next candidate multiplier (variable part)
	// Return values:
	//   True - found next candidate; nVariableMultiplier has the candidate
	//   False - scan complete, no more candidate and reset scan
	bool GetNextCandidateMultiplier(unsigned int& nVariableMultiplier)
	{
		if( commandlineInput.sieveForTwinChains )
		{
			for(;;)
			{
				nCandidateMultiplier++;
				if (nCandidateMultiplier >= nSieveSize)
				{
					nCandidateMultiplier = 0;
					return false;
				}
				uint32 byteIdx = nCandidateMultiplier>>5;
				uint32 mask = 1<<(nCandidateMultiplier&31);

				if (!(vfCompositeCunningham1[byteIdx]&mask) ||
					!(vfCompositeCunningham2[byteIdx]&mask) ||
					!(vfCompositeBiTwin[byteIdx]&mask))
				{
					nVariableMultiplier = nCandidateMultiplier;
					return true;
				}
			}
		}
		else
		{
			for(;;)
			{
				nCandidateMultiplier++;
				if (nCandidateMultiplier >= nSieveSize)
				{
					nCandidateMultiplier = 0;
					return false;
				}
				uint32 byteIdx = nCandidateMultiplier>>5;
				uint32 mask = 1<<(nCandidateMultiplier&31);

				if (!(vfCompositeCunningham1[byteIdx]&mask) ||
					!(vfCompositeCunningham2[byteIdx]&mask) )
				{
					nVariableMultiplier = nCandidateMultiplier;
					return true;
				}
			}
		}
	}

	// Weave the sieve for the next prime in table
	// Return values:
	//   True  - weaved another prime; nComposite - number of composites removed
	//   False - sieve already completed
	//bool WeaveOriginal();
	//bool WeaveFast();
	//bool WeaveFast2();
	bool WeaveFastAll();
	bool WeaveFastAll_noTwin();
	// bool WeaveAlt();
};



#endif
