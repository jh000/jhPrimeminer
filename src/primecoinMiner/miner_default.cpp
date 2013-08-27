#include"global.h"
#include <time.h>

void mpz_set_uint256(mpz_t r, uint256& u);

typedef struct  
{
	uint8* vfCandidatesC1;
	uint8* vfCandidatesC2;
	uint8* vfCandidatesBt;
	// sieve meta
	uint32 nSieveSize;
	uint32 nPrimesToSieve;
	uint32 triedMultiplier;
}pSieve_t;

__declspec( thread ) pSieve_t* thread_pSieve = NULL; 

unsigned int int_invert(unsigned int a, unsigned int nPrime);
extern uint32* vTwoInverses;

void pSieve_prepare(uint32 nBits, uint32 sieveBits, mpz_class* _mpzFixedFactor, uint32 nSieveSize, uint32 nPrimesToSieve)
{
	pSieve_t* pSieve;
	// align sieve size to 64 (so we can process multipliers in 64 bit integers)
	nSieveSize = (nSieveSize+63)&~63;
	// do we need to free the sieve?
	if( thread_pSieve && (thread_pSieve->nSieveSize != nSieveSize) )
	{
		// free sieve - parameters changed
		free(thread_pSieve->vfCandidatesC1);
		free(thread_pSieve->vfCandidatesC2);
		free(thread_pSieve->vfCandidatesBt);
		free(thread_pSieve);
		thread_pSieve = NULL;
	}
	// allocate and init sieve
	if( thread_pSieve == NULL )
	{
		thread_pSieve = (pSieve_t*)malloc(sizeof(pSieve_t));
		memset(thread_pSieve, 0x00, sizeof(pSieve_t));
		pSieve = thread_pSieve;
		pSieve->nSieveSize = nSieveSize;
		pSieve->nPrimesToSieve = nPrimesToSieve;
		pSieve->vfCandidatesC1 = (uint8*)malloc(pSieve->nSieveSize/8 + 1256*2);
		pSieve->vfCandidatesC2 = (uint8*)malloc(pSieve->nSieveSize/8 + 1256*2);
		pSieve->vfCandidatesBt = (uint8*)malloc(pSieve->nSieveSize/8 + 1256*2);
	}
	else
		pSieve = thread_pSieve;

	// make sure primesToSieve is correct (doesn't need full sieve reset)
	pSieve->nPrimesToSieve = nPrimesToSieve;

	memset(pSieve->vfCandidatesC1, 0x00, pSieve->nSieveSize/8);
	memset(pSieve->vfCandidatesC2, 0x00, pSieve->nSieveSize/8);
	memset(pSieve->vfCandidatesBt, 0x00, pSieve->nSieveSize/8);

	const unsigned int nChainLength = TargetGetLength(nBits);
	unsigned int nPrimeSeq = 0;
	//unsigned int vPrimesSize2 = vPrimesSize;

	// Process only 10% of the primes
	// Most composites are still found
	//vPrimesSize2 = (uint64)vPrimesSize2 * 8 / 100;

	pSieve->triedMultiplier = 1;

	mpz_t mpzFixedFactor;


	unsigned long nFixedFactorMod;
	unsigned long nFixedInverse;
	unsigned long nTwoInverse;
	uint32 nTime = GetTickCount();
	mpz_init_set(mpzFixedFactor, _mpzFixedFactor->get_mpz_t());
	uint8* vfCompositeCunningham1 = pSieve->vfCandidatesC1;
	uint8* vfCompositeCunningham2 = pSieve->vfCandidatesC2;
	uint8* vfCompositeBiTwin = pSieve->vfCandidatesBt;

	uint64 bufferedSieveSize = nSieveSize + 1256*8;
	// small primes, cached access
	for(uint32 nPrimeSeq=0; nPrimeSeq<1200; nPrimeSeq++) // 1200 is a good value
	{
		// seems to be buggy :(
		register unsigned int nPrime = vPrimes[nPrimeSeq];
		nFixedFactorMod = mpz_tdiv_ui(mpzFixedFactor, nPrime);
		if (nFixedFactorMod == 0)
		{
			// Nothing in the sieve is divisible by this prime
			continue;
		}
		uint32 pU64 = (uint32)vPrimes[nPrimeSeq];
		uint32 fixedInverseU64 = int_invert(nFixedFactorMod, nPrime);//mpz_get_ui(mpzFixedInverse);
		uint32 twoInverseU64 = vTwoInverses[nPrimeSeq];
		// Weave the sieve for the prime
		uint32 nMultiplierC1[10];
		uint32 nMultiplierC2[10];
		uint32 nMultiplierBt[10];
		for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < 10 * 2; nBiTwinSeq++)
		{
			if( (nBiTwinSeq&1) == 0 )
			{
				uint32 nSolvedMultiplier = fixedInverseU64;
				nMultiplierC1[nBiTwinSeq>>1] = nSolvedMultiplier;
				if( nBiTwinSeq < 10 )
					nMultiplierBt[nBiTwinSeq] = nSolvedMultiplier;
			}
			else
			{
				uint32 nSolvedMultiplier = (pU64-fixedInverseU64);//((fixedInverseU64) * (pU64 - 1ULL)) % pU64;
				nMultiplierC2[nBiTwinSeq>>1] = nSolvedMultiplier;
				if( nBiTwinSeq < 10 )
					nMultiplierBt[nBiTwinSeq] = nSolvedMultiplier;
				fixedInverseU64 = (uint32)(((uint64)fixedInverseU64*(uint64)twoInverseU64)%(uint64)pU64);
			}
		}

		register uint64 nSolvedMultiplier0 = nMultiplierC1[0];
		register uint64 nSolvedMultiplier1 = nMultiplierC1[1];
		register uint64 nSolvedMultiplier2 = nMultiplierC1[2];
		register uint64 nSolvedMultiplier3 = nMultiplierC1[3];
		register uint64 nSolvedMultiplier4 = nMultiplierC1[4];
		register uint64 nSolvedMultiplier5 = nMultiplierC1[5];
		register uint64 nSolvedMultiplier6 = nMultiplierC1[6];
		register uint64 nSolvedMultiplier7 = nMultiplierC1[7];
		register uint64 nSolvedMultiplier8 = nMultiplierC1[8];
		register uint64 nSolvedMultiplier9 = nMultiplierC1[9];
		for (;nSolvedMultiplier0<bufferedSieveSize;)
		{
			vfCompositeCunningham1[nSolvedMultiplier0>>3] |= 1<<(nSolvedMultiplier0&7);
			nSolvedMultiplier0 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier1>>3] |= 1<<(nSolvedMultiplier1&7);
			nSolvedMultiplier1 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier2>>3] |= 1<<(nSolvedMultiplier2&7);
			nSolvedMultiplier2 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier3>>3] |= 1<<(nSolvedMultiplier3&7);
			nSolvedMultiplier3 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier4>>3] |= 1<<(nSolvedMultiplier4&7);
			nSolvedMultiplier4 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier5>>3] |= 1<<(nSolvedMultiplier5&7);
			nSolvedMultiplier5 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier6>>3] |= 1<<(nSolvedMultiplier6&7);
			nSolvedMultiplier6 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier7>>3] |= 1<<(nSolvedMultiplier7&7);
			nSolvedMultiplier7 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier8>>3] |= 1<<(nSolvedMultiplier8&7);
			nSolvedMultiplier8 += nPrime;
			vfCompositeCunningham1[nSolvedMultiplier9>>3] |= 1<<(nSolvedMultiplier9&7);
			nSolvedMultiplier9 += nPrime;
		}
		nSolvedMultiplier0 = nMultiplierC2[0];
		nSolvedMultiplier1 = nMultiplierC2[1];
		nSolvedMultiplier2 = nMultiplierC2[2];
		nSolvedMultiplier3 = nMultiplierC2[3];
		nSolvedMultiplier4 = nMultiplierC2[4];
		nSolvedMultiplier5 = nMultiplierC2[5];
		nSolvedMultiplier6 = nMultiplierC2[6];
		nSolvedMultiplier7 = nMultiplierC2[7];
		nSolvedMultiplier8 = nMultiplierC2[8];
		nSolvedMultiplier9 = nMultiplierC2[9];
		for (;nSolvedMultiplier0<bufferedSieveSize;)
		{
			vfCompositeCunningham2[nSolvedMultiplier0>>3] |= 1<<(nSolvedMultiplier0&7);
			nSolvedMultiplier0 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier1>>3] |= 1<<(nSolvedMultiplier1&7);
			nSolvedMultiplier1 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier2>>3] |= 1<<(nSolvedMultiplier2&7);
			nSolvedMultiplier2 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier3>>3] |= 1<<(nSolvedMultiplier3&7);
			nSolvedMultiplier3 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier4>>3] |= 1<<(nSolvedMultiplier4&7);
			nSolvedMultiplier4 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier5>>3] |= 1<<(nSolvedMultiplier5&7);
			nSolvedMultiplier5 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier6>>3] |= 1<<(nSolvedMultiplier6&7);
			nSolvedMultiplier6 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier7>>3] |= 1<<(nSolvedMultiplier7&7);
			nSolvedMultiplier7 += nPrime;
			vfCompositeCunningham2[nSolvedMultiplier9>>3] |= 1<<(nSolvedMultiplier9&7);
			nSolvedMultiplier9 += nPrime;
		}
		nSolvedMultiplier0 = nMultiplierBt[0];
		nSolvedMultiplier1 = nMultiplierBt[1];
		nSolvedMultiplier2 = nMultiplierBt[2];
		nSolvedMultiplier3 = nMultiplierBt[3];
		nSolvedMultiplier4 = nMultiplierBt[4];
		nSolvedMultiplier5 = nMultiplierBt[5];
		nSolvedMultiplier6 = nMultiplierBt[6];
		nSolvedMultiplier7 = nMultiplierBt[7];
		nSolvedMultiplier8 = nMultiplierBt[8];
		nSolvedMultiplier9 = nMultiplierBt[9];
		for (;nSolvedMultiplier0<bufferedSieveSize;)
		{
			vfCompositeBiTwin[nSolvedMultiplier0>>3] |= 1<<(nSolvedMultiplier0&7);
			nSolvedMultiplier0 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier1>>3] |= 1<<(nSolvedMultiplier1&7);
			nSolvedMultiplier1 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier2>>3] |= 1<<(nSolvedMultiplier2&7);
			nSolvedMultiplier2 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier3>>3] |= 1<<(nSolvedMultiplier3&7);
			nSolvedMultiplier3 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier4>>3] |= 1<<(nSolvedMultiplier4&7);
			nSolvedMultiplier4 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier5>>3] |= 1<<(nSolvedMultiplier5&7);
			nSolvedMultiplier5 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier6>>3] |= 1<<(nSolvedMultiplier6&7);
			nSolvedMultiplier6 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier7>>3] |= 1<<(nSolvedMultiplier7&7);
			nSolvedMultiplier7 += nPrime;
			vfCompositeBiTwin[nSolvedMultiplier9>>3] |= 1<<(nSolvedMultiplier9&7);
			nSolvedMultiplier9 += nPrime;
		}
	}
	// continue with larger primes where the distance does not fall into the same cache line
	for(uint32 nPrimeSeq=1200; nPrimeSeq<pSieve->nPrimesToSieve; nPrimeSeq++)
	{
		register unsigned int nPrime = vPrimes[nPrimeSeq];
		nFixedFactorMod = mpz_tdiv_ui(mpzFixedFactor, nPrime);
		if (nFixedFactorMod == 0)
			continue;
		// Find the modulo inverse of fixed factor
		uint32 fixedInverse = int_invert(nFixedFactorMod, nPrime);
		uint32 twoInverseU64 = vTwoInverses[nPrimeSeq];
		// Weave the sieve for the prime
		uint32 solvedMultiplierC1[10];
		uint32 solvedMultiplierC2[10];
		//uint32 solvedMultiplierBT[10];
		for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < 10; nBiTwinSeq++)
		{
			solvedMultiplierC1[nBiTwinSeq] = fixedInverse;
			solvedMultiplierC2[nBiTwinSeq] = (nPrime-fixedInverse);
			fixedInverse = (uint32)(((uint64)fixedInverse*(uint64)twoInverseU64)%(uint64)nPrime);
		}
		for(uint32 nBiTwinSeq=0; nBiTwinSeq<10; nBiTwinSeq++)
		{
			uint64 nSolvedMultiplier = solvedMultiplierC1[nBiTwinSeq];
			for (register uint64 nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
			{
				_mm_prefetch((const CHAR *)&vfCompositeCunningham1[nVariableMultiplier>>3], _MM_HINT_NTA);
				vfCompositeCunningham1[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			}
		}
		for(uint32 nBiTwinSeq=0; nBiTwinSeq<10; nBiTwinSeq++)
		{
			uint64 nSolvedMultiplier = solvedMultiplierC2[nBiTwinSeq];
			for (register uint64 nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
			{
				_mm_prefetch((const CHAR *)&vfCompositeCunningham2[nVariableMultiplier>>3], _MM_HINT_NTA);
				vfCompositeCunningham2[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			}
		}
		for(uint32 nBiTwinSeq=0; nBiTwinSeq<10; nBiTwinSeq++)
		{
			uint64 nSolvedMultiplier;
			if( nBiTwinSeq & 1 )
				nSolvedMultiplier = solvedMultiplierC2[nBiTwinSeq>>1];
			else
				nSolvedMultiplier = solvedMultiplierC1[nBiTwinSeq>>1];
			for (register uint64 nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
			{
				_mm_prefetch((const CHAR *)&vfCompositeBiTwin[nVariableMultiplier>>3], _MM_HINT_NTA);
				vfCompositeBiTwin[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			}
		}
	}
	mpz_clear(mpzFixedFactor);

	nTime = GetTickCount() - nTime;
	//printf("Sieve time: %dms\n", nTime);

	return;
}

uint32 pSieve_nextMultiplier(uint32* sieveFlags)
{
	*sieveFlags = 0;
	pSieve_t* pSieve = thread_pSieve;
	uint32 multiplier = pSieve->triedMultiplier;
	uint64* vfCandidatesC1_64 = (uint64*)pSieve->vfCandidatesC1;
	uint64* vfCandidatesC2_64 = (uint64*)pSieve->vfCandidatesC2;
	uint64* vfCandidatesBt_64 = (uint64*)pSieve->vfCandidatesBt;
	while( multiplier < pSieve->nSieveSize )
	{
		if( (multiplier&63) == 0 )
		{
			// test 64 candidates at once
			uint32 maskIdx = multiplier>>6;
			if( (vfCandidatesC1_64[maskIdx]&vfCandidatesC2_64[maskIdx]&vfCandidatesBt_64[maskIdx]) == 0xFFFFFFFFFFFFFFFFULL )
			{
				multiplier += 64;
				continue;
			}
		}
		// test individual bits
		uint32 maskVal = (1<<(multiplier&7));
		uint32 maskIdx = multiplier>>3;
		if( (pSieve->vfCandidatesC1[maskIdx]&pSieve->vfCandidatesC2[maskIdx]&pSieve->vfCandidatesBt[maskIdx]&maskVal) == 0 )
		{
			pSieve->triedMultiplier = multiplier+1;
			// for chain type filtering:
			if( pSieve->vfCandidatesC1[maskIdx]&maskVal )
				*sieveFlags |= SIEVE_FLAG_C1_COMPOSITE;
			if( pSieve->vfCandidatesC2[maskIdx]&maskVal )
				*sieveFlags |= SIEVE_FLAG_C2_COMPOSITE;
			if( pSieve->vfCandidatesBt[maskIdx]&maskVal )
				*sieveFlags |= SIEVE_FLAG_BT_COMPOSITE;
			// printf("Multiplier %d\n", multiplier);
			return multiplier;
		}
		multiplier++;
	}
	return 0;
}

__declspec( thread ) uint32 proofOfWork_maxChainlength;
__declspec( thread ) uint32 proofOfWork_nonce;
__declspec( thread ) uint32 proofOfWork_multiplier;
__declspec( thread ) uint32 proofOfWork_depth;
__declspec( thread ) uint32 proofOfWork_chainType; // 0 -> Cunningham first kind, 1 -> Cunningham second kind, 2 -> bitwin

bool MineProbablePrimeChain(primecoinBlock_t* block, mpz_class& mpzFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength,
							unsigned int& nTests, unsigned int& nPrimesHit, sint32 threadIndex, mpz_class& mpzHash, unsigned int nPrimorialMultiplier)
{

	nProbableChainLength = 0;
	nTests = 0;
	nPrimesHit = 0;
	fNewBlock = false;

	mpz_class mpzHashMultiplier = mpzHash * mpzFixedMultiplier;
	mpz_class mpzChainOrigin;

	uint32 nSieveSize = max(block->sievesizeMin, min(block->sievesizeMax, minerSettings.nSieveSize));
	uint32 nPrimesToSieve = max(block->primesToSieveMin, min(block->primesToSieveMax, minerSettings.nPrimesToSieve));
	pSieve_prepare(block->nBits, block->nBits, &mpzHashMultiplier, nSieveSize, nPrimesToSieve);

	// Determine the sequence number of the round primorial
	unsigned int nPrimorialSeq = 0;
	while (vPrimes[nPrimorialSeq + 1] <= nPrimorialMultiplier)
		nPrimorialSeq++;

	// Allocate GMP variables for primality tests
	CPrimalityTestParams testParams(block->nBits, nPrimorialSeq);
	// Compute parameters for fast div test
	{
		//unsigned long lDivisor = 1;
		//unsigned int i;
		//testParams.vFastDivSeq.push_back(nPrimorialSeq);
		//for (i = 1; i <= nFastDivPrimes; i++)
		//{
		//	// Multiply primes together until the result won't fit an unsigned long
		//	if (lDivisor < ULONG_MAX / vPrimes[nPrimorialSeq + i])
		//		lDivisor *= vPrimes[nPrimorialSeq + i];
		//	else
		//	{
		//		testParams.vFastDivisors.push_back(lDivisor);
		//		testParams.vFastDivSeq.push_back(nPrimorialSeq + i);
		//		lDivisor = 1;
		//	}
		//}

		//// Finish off by multiplying as many primes as possible
		//while (lDivisor < ULONG_MAX / vPrimes[nPrimorialSeq + i])
		//{
		//	lDivisor *= vPrimes[nPrimorialSeq + i];
		//	i++;
		//}
		//testParams.vFastDivisors.push_back(lDivisor);
		//testParams.vFastDivSeq.push_back(nPrimorialSeq + i);
		//testParams.nFastDivisorsSize = testParams.vFastDivisors.size();
	}

	// References to counters;
	unsigned int& nChainLengthCunningham1 = testParams.nChainLengthCunningham1;
	unsigned int& nChainLengthCunningham2 = testParams.nChainLengthCunningham2;
	unsigned int& nChainLengthBiTwin = testParams.nChainLengthBiTwin;

	//nStart = GetTickCount();
	//nCurrent = nStart;

	//uint32 timeStop = GetTickCount() + 25000;
	//int nTries = 0;
	//bool multipleShare = false;




	

	uint32 sieveFlags;
	uint32 numMultipliers = 0;
	uint32 goodChains = 0;
	primeStats.numAllTestedNumbers += (minerSettings.nSieveSize);
	uint32 prevMultiplier;
	while ( (block->xptFlags&XPT_WORKBUNDLE_FLAG_INTERRUPTABLE) || block->blockHeight == jhMiner_getCurrentWorkBlockHeight(block->threadIndex) )
	{
		prevMultiplier = nTriedMultiplier;
		nTriedMultiplier = pSieve_nextMultiplier(&sieveFlags);
		//nTries++;
		nTests++;
		if (nTriedMultiplier == 0 )
		{
			//printf("numMultipliers: %4d goodChains: %4d\n", numMultipliers, goodChains);
			return false;
		}
		mpzChainOrigin = mpzHashMultiplier * nTriedMultiplier;
		// test sieve C2
		//if( (sieveFlags&SIEVE_FLAG_C2_COMPOSITE)==0 )
		//{
		//	// -1 is not composite
		//	mpz_class mpzF;
		//	mpzF = mpzChainOrigin - 1;
		//	for(uint32 f=0; f<9; f++)
		//	{
		//		for(uint32 n=0; n<minerSettings.nPrimesToSieve; n++)
		//		{
		//			uint32 nPrimeMod = mpz_tdiv_ui(mpzF.get_mpz_t(), vPrimes[n]);
		//			if( nPrimeMod == 0 )
		//			{
		//				char tmpStr[1024];
		//				mpz_get_str(tmpStr, 10, mpzChainOrigin.get_mpz_t());
		//				printf("mpzChainOrigin:\n%s\n", tmpStr);
		//				__debugbreak();
		//			}
		//		}
		//		mpzF = mpzF + mpzF;
		//		mpzF += 1;
		//	}
		//}
		//// test sieve C1
		//if( (sieveFlags&SIEVE_FLAG_C1_COMPOSITE)==0 )
		//{
		//	// +1 is not composite
		//	mpz_class mpzF;
		//	mpzF = mpzChainOrigin + 1;
		//	for(uint32 f=0; f<9; f++)
		//	{
		//		for(uint32 n=0; n<minerSettings.nPrimesToSieve; n++)
		//		{
		//			uint32 nPrimeMod = mpz_tdiv_ui(mpzF.get_mpz_t(), vPrimes[n]);
		//			if( nPrimeMod == 0 )
		//			{
		//				char tmpStr[1024];
		//				mpz_get_str(tmpStr, 10, mpzChainOrigin.get_mpz_t());
		//				printf("mpzChainOrigin:\n%s\n", tmpStr);
		//				__debugbreak();
		//			}
		//		}
		//		mpzF = mpzF + mpzF;
		//		mpzF -= 1;
		//	}
		//}
		// inverted test -> make sure we dont miss candidates from the sieve
		//if( (sieveFlags&SIEVE_FLAG_C2_COMPOSITE)!=0 )
		//{
		//	for(uint32 i=(prevMultiplier+1); i<nTriedMultiplier; i++)
		//	{
		//		//printf("%d\n", i);
		//		if( i == 0 )
		//			continue;
		//		// -1 is not composite
		//		mpz_class mpzF;
		//		mpzF = mpzChainOrigin + 1;
		//		bool isDivisible = false;
		//		for(uint32 f=0; f<10; f++)
		//		{
		//			for(uint32 n=0; n<minerSettings.nPrimesToSieve; n++)
		//			{
		//				uint32 nPrimeMod = mpz_tdiv_ui(mpzF.get_mpz_t(), vPrimes[n]);
		//				if( nPrimeMod == 0 )
		//				{
		//					char tmpStr[1024];
		//					mpz_get_str(tmpStr, 10, mpzChainOrigin.get_mpz_t());
		//					//printf("mpzChainOrigin:\n%s\n", tmpStr);
		//					isDivisible = true;
		//				}
		//			}
		//			mpzF = mpzF + mpzF;
		//			mpzF -= 1;
		//		}
		//		if( isDivisible == false )
		//			__debugbreak();
		//	}
		//}

		nChainLengthCunningham1 = 0;
		nChainLengthCunningham2 = 0;
		nChainLengthBiTwin = 0;
		sieveFlags = 0; // special chain type filtering disabled, generates less shares, makes users unhappy with the current payout model :(
		bool canSubmitAsShare = ProbablePrimeChainTestFast(mpzChainOrigin, testParams, sieveFlags);
		//CBigNum bnChainOrigin;
		//bnChainOrigin.SetHex(mpzChainOrigin.get_str(16));
		//bool canSubmitAsShare = ProbablePrimeChainTestBN(bnChainOrigin, block->serverData.nBitsForShare, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin);

		nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);
		//printf("%04d: %08X\n", numMultipliers, nProbableChainLength);
		//if( numMultipliers == 20 )
		//{
		//	//printf("goodChains: %d\n", goodChains);
		//	if( goodChains == 0 )
		//		break;
		//}
		if( nProbableChainLength >= proofOfWork_maxChainlength )
		{
			proofOfWork_maxChainlength = nProbableChainLength;
			proofOfWork_multiplier = nTriedMultiplier;
			proofOfWork_nonce = block->nonce;
			proofOfWork_depth = 0; // doing no depth scan (multipass sieve only)
			// get chaintype
			if( nChainLengthCunningham1 >= nProbableChainLength )
				proofOfWork_chainType = 0; // Cunningham first kind
			else if( nChainLengthCunningham2 >= nProbableChainLength )
				proofOfWork_chainType = 1; // Cunningham first kind
			else
				proofOfWork_chainType = 2; // bitwin chain
		}
		if( nProbableChainLength >= 0x03000000 )
		{
			goodChains++;
			//primeStats.nChainHit += pow(10, ((float)((double)nProbableChainLength / (double)0x1000000))-7.0);
			//primeStats.nChainHit += pow(8, floor((float)((double)nProbableChainLength / (double)0x1000000)) - 7);
			if (nProbableChainLength >= 0x7000000)
				primeStats.sevenChainCount ++;
			else if (nProbableChainLength >= 0x6000000)
				primeStats.sixChainCount ++;
			else if (nProbableChainLength >= 0x5000000)
				primeStats.fiveChainCount ++;
			else if (nProbableChainLength >= 0x4000000)
			{
				nTests = 0;
				primeStats.fourChainCount ++;
			}
		}
		//if( nBitsGen >= 0x03000000 )
		// printf("%08X\n", nBitsGen);
		primeStats.primeChainsFound++;
		//if( nProbableChainLength > 0x03000000 )
		// primeStats.qualityPrimesFound++;
		if( nProbableChainLength > primeStats.bestPrimeChainDifficulty )
			primeStats.bestPrimeChainDifficulty = nProbableChainLength;

		if(nProbableChainLength >= block->nBitsForShare)
		{
			//primeStats.shareFound = true;

			block->mpzPrimeChainMultiplier = mpzFixedMultiplier * nTriedMultiplier;
			// generate block raw data
			uint8 blockRawData[256] = {0};
			memcpy(blockRawData, block, 80);
			uint32 writeIndex = 80;
			sint32 lengthBN = 0;
			CBigNum bnPrimeChainMultiplier;
			bnPrimeChainMultiplier.SetHex(block->mpzPrimeChainMultiplier.get_str(16));
			std::vector<unsigned char> bnSerializeData = bnPrimeChainMultiplier.getvch();
			lengthBN = bnSerializeData.size();
			*(uint8*)(blockRawData+writeIndex) = (uint8)lengthBN; // varInt (we assume it always has a size low enough for 1 byte)
			writeIndex += 1;
			memcpy(blockRawData+writeIndex, &bnSerializeData[0], lengthBN);
			writeIndex += lengthBN;	
			// switch endianness
			for(uint32 f=0; f<256/4; f++)
			{
				*(uint32*)(blockRawData+f*4) = _swapEndianessU32(*(uint32*)(blockRawData+f*4));
			}
			time_t now = time(0);
			struct tm * timeinfo;
			timeinfo = localtime (&now);
			char sNow [80];
			strftime (sNow, 80, "%x - %X",timeinfo);
			printf("%s - SHARE FOUND !!! (Th#: %u) --- DIFF: %f %s %s\n",
				sNow, threadIndex, (float)((double)nProbableChainLength / (double)0x1000000),
				nProbableChainLength >= 0x6000000 ? ">6":"", nProbableChainLength >= 0x7000000 ? ">7":"");
			// submit this share
			if (jhMiner_pushShare_primecoin(blockRawData, block))
				primeStats.foundShareCount ++;
			//printf("Probable prime chain found for block=%s!!\n Target: %s\n Length: (%s %s %s)\n", block.GetHash().GetHex().c_str(),TargetToString(block.nBits).c_str(), TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
			//nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);
			// since we are using C structs here we have to make sure the memory for the CBigNum in the block is freed again
			//delete *psieve;
			//*psieve = NULL;
			//block->bnPrimeChainMultiplier = NULL;
			RtlZeroMemory(blockRawData, 256);
			//delete *psieve;
			//*psieve = NULL;
			// dont quit if we find a share, there could be other shares in the remaining prime candidates
			nTests = 0; // tehere is a good chance to find more shares so search a litle more.
			//block->nonce++;
			//return true;
			//break;
			//if (multipleShare)
		}
		//if(TargetGetLength(nProbableChainLength) >= 1)
		// nPrimesHit++;
		//nCurrent = GetTickCount();
		numMultipliers++;
	}
	//printf("numMultipliers: %d\n", numMultipliers);

	return false; // stop as timed out
}

void BitcoinMiner(primecoinBlock_t* primecoinBlock, sint32 threadIndex)
{
	//printf("PrimecoinMiner started\n");
	//SetThreadPriority(THREAD_PRIORITY_LOWEST);
	//RenameThread("primecoin-miner");
	if( pctx == NULL )
		pctx = BN_CTX_new();
	// Each thread has its own key and counter
	//CReserveKey reservekey(pwallet);
	unsigned int nExtraNonce = 0;

	//static const unsigned int nPrimorialHashFactor = 7;
	unsigned int nPrimorialMultiplier = primecoinBlock->fixedPrimorial;
	int64 nTimeExpected = 0;   // time expected to prime chain (micro-second)
	int64 nTimeExpectedPrev = 0; // time expected to prime chain last time
	bool fIncrementPrimorial = true; // increase or decrease primorial factor
	int64 nSieveGenTime = 0;

	//primecoinBlock->nonce = 0;
	//TODO: check this if it makes sense
	if( primecoinBlock->xptMode == false )
		primecoinBlock->nonce = 0x00010000 * threadIndex;
	else
		primecoinBlock->nonce = primecoinBlock->nonceMin;
	//primecoinBlock->nonce = 0;

	uint32 nTime = GetTickCount() + 1000*600;

	uint32 nStatTime = GetTickCount() + 2000;

	uint32 loopCount = 0;
	unsigned int nHashFactor;
	if( primecoinBlock->fixedHashFactor )
		nHashFactor = PrimorialFast(primecoinBlock->fixedHashFactor);
	else
		nHashFactor = 0;

	//uint32 nTimeRollStart = primecoinBlock->timestamp;


	// start proof of work generation
	uint32 nSieveSize = max(primecoinBlock->sievesizeMin, min(primecoinBlock->sievesizeMax, minerSettings.nSieveSize));
	uint32 nPrimesToSieve = max(primecoinBlock->primesToSieveMin, min(primecoinBlock->primesToSieveMax, minerSettings.nPrimesToSieve));
	jhMiner_primecoinBeginProofOfWork(primecoinBlock->merkleRoot, primecoinBlock->prevBlockHash, nSieveSize, nPrimesToSieve, primecoinBlock->timestamp);
	
	// reset proof of work hash
	proofOfWork_maxChainlength = 0;
	uint32 numberOfProcessedSieves = 0;

	mpz_class mpzPrimorial;
	Primorial(nPrimorialMultiplier, mpzPrimorial);
	while( true )
	{
		if( primecoinBlock->xptFlags & XPT_WORKBUNDLE_FLAG_INTERRUPTABLE )
		{
			if( primecoinBlock->blockHeight != jhMiner_getCurrentWorkBlockHeight(primecoinBlock->threadIndex) )
				break;
		}
		primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
		//
		// Search
		//
		bool fNewBlock = true;
		unsigned int nTriedMultiplier = 0;
		// Primecoin: try to find hash divisible by first few primes of primorial
		uint256 phash = primecoinBlock->blockHeaderHash;
		mpz_class mpzHash;
		mpz_set_uint256(mpzHash.get_mpz_t(), phash);

		if( nHashFactor > 0 ) 
		{
			while ((phash < hashBlockHeaderLimit || !mpz_divisible_ui_p(mpzHash.get_mpz_t(), nHashFactor)) && primecoinBlock->nonce < primecoinBlock->nonceMax) {
				primecoinBlock->nonce++;
				primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
				phash = primecoinBlock->blockHeaderHash;
				mpz_set_uint256(mpzHash.get_mpz_t(), phash);
			}
		}
		else
		{
			while ((phash < hashBlockHeaderLimit || !mpz_probab_prime_p(mpzHash.get_mpz_t(), 8)) && primecoinBlock->nonce < primecoinBlock->nonceMax) {
				primecoinBlock->nonce++;
				primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
				phash = primecoinBlock->blockHeaderHash;
				mpz_set_uint256(mpzHash.get_mpz_t(), phash);
			}
		}
		// did we reach the end of the nonce range?
		if (primecoinBlock->nonce >= primecoinBlock->nonceMax)
		{
			jhMiner_primecoinAddProofOfWork(primecoinBlock->merkleRoot, primecoinBlock->prevBlockHash, numberOfProcessedSieves, primecoinBlock->timestamp, proofOfWork_maxChainlength, proofOfWork_chainType, proofOfWork_nonce, proofOfWork_multiplier, proofOfWork_depth);
			numberOfProcessedSieves = 0;
			proofOfWork_maxChainlength = 0;
			proofOfWork_multiplier = 0;
			proofOfWork_nonce = 0;
			proofOfWork_depth = 0;
			if( (primecoinBlock->xptFlags & XPT_WORKBUNDLE_FLAG_TIMESTAMPROLL) == 0 )
				break;
			// do timestamp roll (xpt rule: Dont skip timestamp values)
			primecoinBlock->timestamp++;
			primecoinBlock->nonce = primecoinBlock->nonceMin;
		}
		// Primecoin: primorial fixed multiplier
		unsigned int nRoundTests = 0;
		unsigned int nRoundPrimesHit = 0;
		int64 nPrimeTimerStart = GetTickCount();

		//if( loopCount > 0 )
		//{
		//	//primecoinBlock->nonce++;
		//	if (!PrimeTableGetNextPrime(nPrimorialMultiplier))
		//		error("PrimecoinMiner() : primorial increment overflow");
		//}

		unsigned int nTests = 0;
		unsigned int nPrimesHit = 0;

		//mpz_class mpzMultiplierMin = mpzPrimeMin * nHashFactor / mpzHash + 1;
		//while (mpzPrimorial < mpzMultiplierMin )
		//{
		//	if (!PrimeTableGetNextPrime(nPrimorialMultiplier))
		//		error("PrimecoinMiner() : primorial minimum overflow");
		//	Primorial(nPrimorialMultiplier, mpzPrimorial);
		//}
		mpz_class mpzFixedMultiplier;
		mpzFixedMultiplier = mpzPrimorial / nHashFactor;
		//printf("fixedMultiplier: %d nPrimorialMultiplier: %d\n", BN_get_word(&bnFixedMultiplier), nPrimorialMultiplier);
		// Primecoin: mine for prime chain
		unsigned int nProbableChainLength;
		MineProbablePrimeChain(primecoinBlock, mpzFixedMultiplier, fNewBlock, nTriedMultiplier, nProbableChainLength, nTests, nPrimesHit, threadIndex, mpzHash, nPrimorialMultiplier);
		numberOfProcessedSieves++;
		//psieve = NULL;
		nRoundTests += nTests;
		nRoundPrimesHit += nPrimesHit;
		primecoinBlock->nonce++;
		// accurate time roll has been disabled (use timestamp update)
		//uint32 newTimestamp = nTimeRollStart + (unsigned int)time(NULL)-nTimeRollBase;
		//if( newTimestamp != primecoinBlock->timestamp )
		//{
		//	primecoinBlock->timestamp = newTimestamp;
		//	if( primecoinBlock->xptMode == false )
		//		primecoinBlock->nonce = 0x00010000 * threadIndex;
		//	else
		//		primecoinBlock->nonce = 1;
		//}
		loopCount++;
	}
	jhMiner_primecoinCompleteProofOfWork(primecoinBlock->merkleRoot, primecoinBlock->prevBlockHash, primecoinBlock->nonce, numberOfProcessedSieves, primecoinBlock->timestamp, proofOfWork_maxChainlength, proofOfWork_chainType, proofOfWork_nonce, proofOfWork_multiplier, proofOfWork_depth);
}