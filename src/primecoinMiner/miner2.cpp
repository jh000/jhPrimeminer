#include"global.h"

typedef struct  
{
	//uint32 base;
	//uint32 d;
	uint32 remain;
	uint16 idx;
	uint16 limit;
}primeFactor_t;

int single_modinv (int a, int modulus);

typedef struct  
{
	//uint32 p;
	uint32 base;
	uint32 mod;
	uint32 twoInverse;
}cPrimeFactor_t;

typedef struct  
{
	uint32			numPrimeFactors; // how many prime factors are filtered
	cPrimeFactor_t*	primeFactorBase;
	uint32			chainLength; // equal to number of layers
	uint8*			layerMaskC1;
	uint8*			layerMaskC2;
	//uint32**		layerMaskBT;
	uint32			maskBytes;
	uint32			sieveSize;
	// multiplier scan
	uint32			currentMultiplierScanIndex;
	uint32			currentSieveLayerIdx;
	// mpz
	mpz_class		mpzFixedMultiplier;
}continuousSieve_t;

__declspec( thread ) continuousSieve_t* cSieve = NULL;

void cSieve_prepare(mpz_class& mpzFixedMultiplier, uint32 chainLength)
{
	uint32 sieveSize = (nMaxSieveSize+7)&~7; // align to 8
	// todo: Make sure the sieve doesnt crash when the difficulty changes
	if( cSieve == NULL || cSieve->chainLength != chainLength )
	{
		if( cSieve == NULL )
		{
			cSieve = (continuousSieve_t*)malloc(sizeof(continuousSieve_t));
			memset(cSieve, 0x00, sizeof(continuousSieve_t));
		}
		// free bitmasks if the size changed
		if( cSieve->chainLength != chainLength )
		{
			if( cSieve->layerMaskC1 )
			{
				free(cSieve->layerMaskC1);
				free(cSieve->layerMaskC2);
				//free(cSieve->layerMaskBT);
				cSieve->layerMaskC1 = NULL;
				cSieve->layerMaskC2 = NULL;
				//cSieve->layerMaskBT = NULL;
			}
		}
		// alloc sieve mask
		cSieve->sieveSize = sieveSize;
		cSieve->chainLength = chainLength;
		cSieve->maskBytes = (sieveSize+7)/8;
		uint32 chainMaskSize = cSieve->maskBytes * chainLength;
		if( cSieve->layerMaskC1 == NULL )
		{
			cSieve->layerMaskC1 = (uint8*)malloc(chainMaskSize);
			cSieve->layerMaskC2 = (uint8*)malloc(chainMaskSize);
			//cSieve->layerMaskBT = (uint8*)malloc(chainMaskSize);
		}
		// reset sieve
		memset(cSieve->layerMaskC1, 0x00, chainMaskSize);
		memset(cSieve->layerMaskC2, 0x00, chainMaskSize);
		//memset(cSieve->layerMaskBT, 0x00, chainMaskSize);
	}
	// alloc prime factor base table
	if( cSieve->primeFactorBase == NULL )
	{
		unsigned int nPrimes = 0;
		if( nSievePercentage <= 100 )
			nPrimes = (uint64)vPrimes.size() * (uint64)nSievePercentage / 100ULL;
		else
			nPrimes = vPrimes.size(); // use the whole array to avoid rounding problems and out-of-bounds access
		cSieve->numPrimeFactors = nPrimes;//50000;
		cSieve->primeFactorBase = (cPrimeFactor_t*)malloc(sizeof(cPrimeFactor_t)*cSieve->numPrimeFactors);
		for(uint32 i=0; i<cSieve->numPrimeFactors; i++)
		{
			cSieve->primeFactorBase[i].twoInverse = single_modinv(2, vPrimes[i]);
		}
	}
	// calculate fixed inverse
	mpz_class mpzF, mpzTemp;
	for(uint32 i=0; i<cSieve->numPrimeFactors; i++)
	{
		uint32 p = vPrimes[i];
		uint32 fixedFactorMod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzFixedMultiplier.get_mpz_t(), p);
		cSieve->primeFactorBase[i].mod = fixedFactorMod; 
		cSieve->primeFactorBase[i].base = single_modinv(fixedFactorMod, p); 
	}
	// do sieving for the first nChainLength layers
	//for(uint32 t=0; t<cSieve->chainLength; t++)
	//for(uint32 t=0; t<cSieve->chainLength; t++)
	cSieve->mpzFixedMultiplier = mpzFixedMultiplier;
	for(uint32 t=0; t<cSieve->chainLength; t++)
	{
		mpzF = mpzFixedMultiplier;
		uint8* maskC1 = cSieve->layerMaskC1+t*cSieve->maskBytes; 
		uint8* maskC2 = cSieve->layerMaskC2+t*cSieve->maskBytes; 
		for(uint32 i=0; i<cSieve->numPrimeFactors; i++)
		{
			if( cSieve->primeFactorBase[i].mod == 0 )
				continue;
			uint32 p = vPrimes[i];
			/*uint32 fixedFactorMod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzF.get_mpz_t(), p);
			uint32 fixedInverseU32 = single_modinv(fixedFactorMod, p); 
			*///uint32 fixedInverseTwo = single_modinv(2, p);
			
			// mark all cunningham chains of first kind
			uint32 nSolvedMultiplier = cSieve->primeFactorBase[i].base;
			while( nSolvedMultiplier < cSieve->sieveSize )
			{
				maskC1[nSolvedMultiplier>>3] |= (1<<(nSolvedMultiplier&7));
				nSolvedMultiplier += p;
			}
			// mark all cunningham chains of second kind
			uint64 nSolvedMultiplier64 = (uint64)cSieve->primeFactorBase[i].base * (uint64)(p-1); // tested and correct
			nSolvedMultiplier64 %= (uint64)p;

			nSolvedMultiplier = (uint32)nSolvedMultiplier64;
			while( nSolvedMultiplier < cSieve->sieveSize )
			{
				maskC2[nSolvedMultiplier>>3] |= (1<<(nSolvedMultiplier&7));			
				nSolvedMultiplier += p;
			}
			// apply *2 factor to prime base
			//cSieve->primeFactorBase[i].base = (cSieve->primeFactorBase[i].base*cSieve->primeFactorBase[i].twoInverse)%p;
			cSieve->primeFactorBase[i].base = (uint32)(((uint64)cSieve->primeFactorBase[i].base*(uint64)cSieve->primeFactorBase[i].twoInverse)%(uint64)p);
		}
	}
	// reset multiplier scan
	cSieve->currentSieveLayerIdx = 0;
	cSieve->currentMultiplierScanIndex = 1;
}

bool cSieve_sieveNextLayer()
{
	if( cSieve->currentSieveLayerIdx > 64 )
		return false;
	uint32 layerIdx = cSieve->currentSieveLayerIdx % cSieve->chainLength;
	uint8* maskC1 = cSieve->layerMaskC1+layerIdx*cSieve->maskBytes; 
	uint8* maskC2 = cSieve->layerMaskC2+layerIdx*cSieve->maskBytes;
	memset(maskC1, 0x00, cSieve->maskBytes);
	memset(maskC2, 0x00, cSieve->maskBytes);
	for(uint32 i=0; i<cSieve->numPrimeFactors; i++)
	{
		if( cSieve->primeFactorBase[i].mod == 0 )
			continue;
		uint32 p = vPrimes[i];
		// mark all cunningham chains of first kind
		uint32 nSolvedMultiplier = cSieve->primeFactorBase[i].base;
		while( nSolvedMultiplier < cSieve->sieveSize )
		{
			maskC1[nSolvedMultiplier>>3] |= (1<<(nSolvedMultiplier&7));
			nSolvedMultiplier += p;
		}
		// mark all cunningham chains of second kind
		uint64 nSolvedMultiplier64 = (uint64)cSieve->primeFactorBase[i].base * (uint64)(p-1);
		nSolvedMultiplier64 %= (uint64)p;
		nSolvedMultiplier = (uint32)nSolvedMultiplier64;
		while( nSolvedMultiplier < cSieve->sieveSize )
		{
			maskC2[nSolvedMultiplier>>3] |= (1<<(nSolvedMultiplier&7));			
			nSolvedMultiplier += p;
		}
		// apply *2 factor to prime base
		//cSieve->primeFactorBase[i].base = (cSieve->primeFactorBase[i].base*cSieve->primeFactorBase[i].twoInverse)%p;
		cSieve->primeFactorBase[i].base = (uint32)(((uint64)cSieve->primeFactorBase[i].base*(uint64)cSieve->primeFactorBase[i].twoInverse)%(uint64)p);
	}
	cSieve->currentSieveLayerIdx++;
	cSieve->currentMultiplierScanIndex = 1;
	cSieve->mpzFixedMultiplier = cSieve->mpzFixedMultiplier * 2;
	return true;
}

uint32 cSieve_findNextMultiplier(uint32* multiplierSieveFlags)
{
	*multiplierSieveFlags = 0;
	uint32 multiplier = cSieve->currentMultiplierScanIndex;
	if( cSieve->currentSieveLayerIdx & 1 )
		multiplier |= 1; // make sure we only process odd multipliers on every odd layer
	if( cSieve->chainLength >= 64 )
	{
		printf("cSieve_findNextMultiplier(): fatal error, difficulty exceeds allowed value\n");
		ExitProcess(0);
	}
	uint8* maskPtrC1[32]; // works unless difficulty is greater than 32
	uint8* maskPtrC2[32];
	for(uint32 i=0; i<cSieve->chainLength; i++)
	{
		maskPtrC1[i] = cSieve->layerMaskC1+i*cSieve->maskBytes;
		maskPtrC2[i] = cSieve->layerMaskC2+i*cSieve->maskBytes;
	}
	uint32 btn = cSieve->chainLength/2;
	uint32 sieveLayerIdx = cSieve->currentSieveLayerIdx;
	while( true )
	{
		if( multiplier >= cSieve->sieveSize )
		{
			cSieve->currentMultiplierScanIndex = cSieve->sieveSize;
			return 0;
		}
		// get mask for this byte
		uint32 compositeMaskIdx = multiplier>>3;
		// bitwin mask part
		uint8 maskBtC1 = 0;
		uint8 maskBtC2 = 0;
		for(uint32 i=0; i<btn; i++)
		{
			maskBtC1 |= maskPtrC1[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
			maskBtC2 |= maskPtrC2[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
			// note: We ignore the fact that we should actually look for 9 length, not 8 due to 9/2 = 4
		}
		// remaining mask
		uint8 maskC1 = 0;
		uint8 maskC2 = 0;
		for(uint32 i=btn; i<cSieve->chainLength; i++)
		{
			maskC1 |= maskPtrC1[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
			maskC2 |= maskPtrC2[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
		}
		maskC1 |= maskBtC1;
		maskC2 |= maskBtC2;
		uint32 maskBt = maskBtC1|maskBtC2;
		// should we skip parsing this mask?
		if( maskC1 == 0xFF && maskC2 == 0xFF && maskBt == 0xFF )
		{
			multiplier = (multiplier+7)&~7;
			multiplier = multiplier + 8;
			continue;
		}
		// parse the mask bits
		uint32 multParse = multiplier - (multiplier&~7);
		multParse = 8 - multParse;
		for(uint32 t=0; t<multParse; t++)
		{
			// process until aligned to 8
			if( multiplier >= cSieve->sieveSize )
			{
				cSieve->currentMultiplierScanIndex = cSieve->sieveSize;
				return 0;
			}
			uint8 compositeMask = 1<<(multiplier&7);

			if( (maskBt&maskC1&maskC2&compositeMask)==0 )
			{
				// good multiplier found
				if( maskBt&compositeMask )
					*multiplierSieveFlags |= SIEVE_FLAG_BT_COMPOSITE;
				if( maskC1&compositeMask )
					*multiplierSieveFlags |= SIEVE_FLAG_C1_COMPOSITE;
				if( maskC2&compositeMask )
					*multiplierSieveFlags |= SIEVE_FLAG_C2_COMPOSITE;
				cSieve->currentMultiplierScanIndex = multiplier+1;
				return multiplier;
			}

			//if( (maskBt&compositeMask)==0 )
			//{
			//	// good multiplier found
			//	if( maskBt&compositeMask )
			//		*multiplierSieveFlags |= 0; // none is really composite
			//	cSieve->currentMultiplierScanIndex = multiplier+1;
			//	return multiplier;
			//}
			//else if( (maskC1&compositeMask)==0 )
			//{
			//	// good multiplier found
			//	if( maskC2&compositeMask )
			//		*multiplierSieveFlags |= SIEVE_FLAG_C2_COMPOSITE | SIEVE_FLAG_BT_COMPOSITE;
			//	cSieve->currentMultiplierScanIndex = multiplier+1;
			//	return multiplier;
			//}
			//else if( (maskC2&compositeMask)==0 )
			//{
			//	// good multiplier found
			//	if( maskC1&compositeMask )
			//		*multiplierSieveFlags |= SIEVE_FLAG_C1_COMPOSITE | SIEVE_FLAG_BT_COMPOSITE;
			//	cSieve->currentMultiplierScanIndex = multiplier+1;
			//	return multiplier;
			//}
			if( cSieve->currentSieveLayerIdx & 1 )
				multiplier += 2;
			else
				multiplier++;
		}
	}
	return 0;
}

bool ProbableCunninghamChainTestFast(const mpz_class& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength, CPrimalityTestParams& testParams);

// Test probable prime chain for: nOrigin
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
bool ProbablePrimeChainTestFast2(const mpz_class& mpzPrimeChainOrigin, CPrimalityTestParams& testParams, uint32 sieveFlags, uint32 multiplier)
{
	const unsigned int nBits = testParams.nBits;
	unsigned int& nChainLengthCunningham1 = testParams.nChainLengthCunningham1;
	unsigned int& nChainLengthCunningham2 = testParams.nChainLengthCunningham2;
	unsigned int& nChainLengthBiTwin = testParams.nChainLengthBiTwin;
	const bool fFermatTest = testParams.fFermatTest;
	mpz_class& mpzOriginMinusOne = testParams.mpzOriginMinusOne;
	mpz_class& mpzOriginPlusOne = testParams.mpzOriginPlusOne;
	nChainLengthCunningham1 = 0;
	nChainLengthCunningham2 = 0;
	nChainLengthBiTwin = 0;

	bool testCFirstKind = ((sieveFlags&SIEVE_FLAG_BT_COMPOSITE)!=0) || ((sieveFlags&SIEVE_FLAG_C2_COMPOSITE)!=0); // yes, C1 and C2 is switched
	bool testCSecondKind = ((sieveFlags&SIEVE_FLAG_BT_COMPOSITE)!=0) || ((sieveFlags&SIEVE_FLAG_C1_COMPOSITE)!=0);

	// Test for Cunningham Chain of first kind
	mpzOriginMinusOne = mpzPrimeChainOrigin - 1;
	if( testCFirstKind )
		ProbableCunninghamChainTestFast(mpzOriginMinusOne, true, fFermatTest, nChainLengthCunningham1, testParams);
	else
		nChainLengthCunningham1 = 0;
	// Test for Cunningham Chain of second kind
	mpzOriginPlusOne = mpzPrimeChainOrigin + 1;
	if( testCSecondKind )
		ProbableCunninghamChainTestFast(mpzOriginPlusOne, false, fFermatTest, nChainLengthCunningham2, testParams);
	else
		nChainLengthCunningham2 = 0;
	//// do we need to flag some bits as composite to avoid redundant chain checks?
	//uint32 redundancyCheckLength = min(nChainLengthCunningham1, nChainLengthCunningham2);
	//redundancyCheckLength >>= 24;
	//if( redundancyCheckLength >= 2 ) 
	//{
	//	printf(".");
	//	uint32 mask = 1<<(multiplier&7);
	//	uint32 maskIdx = multiplier>>3;
	//	for(uint32 r=0; r<redundancyCheckLength; r++)
	//	{
	//		uint8* maskC1 = cSieve->layerMaskC1+((cSieve->currentSieveLayerIdx+r)%cSieve->chainLength)*cSieve->maskBytes;
	//		uint8* maskC2 = cSieve->layerMaskC1+((cSieve->currentSieveLayerIdx+r)%cSieve->chainLength)*cSieve->maskBytes;
	//		maskC1[maskIdx] |= mask;
	//		maskC2[maskIdx] |= mask;
	//	}
	//}
	// verify if there is any chance to find a biTwin that worth calculation
	if (nChainLengthCunningham1 < 0x2000000 || nChainLengthCunningham2 < 0x2000000)
		return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits);
	// Figure out BiTwin Chain length
	// BiTwin Chain allows a single prime at the end for odd length chain
	nChainLengthBiTwin =
		(TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
		(nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
	(nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));

	return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
}

bool BitcoinMiner2_mineProbableChain(primecoinBlock_t* block, mpz_class& mpzFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, 
										 unsigned int& nTests, unsigned int& nPrimesHit, sint32 threadIndex, mpz_class& mpzHash, unsigned int nPrimorialMultiplier)
{
	mpz_class mpzChainOrigin;
	mpz_class mpzFinalFixedMultiplier = mpzFixedMultiplier * mpzHash;
	mpz_class mpzTemp;
	uint32 nTime = GetTickCount();
	cSieve_prepare(mpzFinalFixedMultiplier, block->nBits>>24);
	nTime = GetTickCount()-nTime;
	//printf("cSieve prep time: %d\n", nTime);

	unsigned int nPrimorialSeq = 0;
	while (vPrimes[nPrimorialSeq + 1] <= nPrimorialMultiplier)
		nPrimorialSeq++;
	// Allocate GMP variables for primality tests
	CPrimalityTestParams testParams(block->nBits, nPrimorialSeq);
	{
		unsigned long lDivisor = 1;
		unsigned int i;
		testParams.vFastDivSeq.push_back(nPrimorialSeq);
		for (i = 1; i <= nFastDivPrimes; i++)
		{
			// Multiply primes together until the result won't fit an unsigned long
			if (lDivisor < ULONG_MAX / vPrimes[nPrimorialSeq + i])
				lDivisor *= vPrimes[nPrimorialSeq + i];
			else
			{
				testParams.vFastDivisors.push_back(lDivisor);
				testParams.vFastDivSeq.push_back(nPrimorialSeq + i);
				lDivisor = 1;
			}
		}

		// Finish off by multiplying as many primes as possible
		while (lDivisor < ULONG_MAX / vPrimes[nPrimorialSeq + i])
		{
			lDivisor *= vPrimes[nPrimorialSeq + i];
			i++;
		}
		testParams.vFastDivisors.push_back(lDivisor);
		testParams.vFastDivSeq.push_back(nPrimorialSeq + i);
		testParams.nFastDivisorsSize = testParams.vFastDivisors.size();
	}
	// References to counters;
	unsigned int& nChainLengthCunningham1 = testParams.nChainLengthCunningham1;
	unsigned int& nChainLengthCunningham2 = testParams.nChainLengthCunningham2;
	unsigned int& nChainLengthBiTwin = testParams.nChainLengthBiTwin;

	uint32 debugStats_primes = 0;
	uint32 debugStats_multipliersTested = 0;
	
	while( block->serverData.blockHeight == jhMiner_getCurrentWorkBlockHeight(block->threadIndex) )
	{
		uint32 sieveFlags;
		uint32 multiplier = cSieve_findNextMultiplier(&sieveFlags);
		if( multiplier == 0 )
		{
			// mix in next layer
			//printf("Layer finished [%d]\n", cSieve->currentSieveLayerIdx);
			if( cSieve_sieveNextLayer() == false )
				break;
			mpzFinalFixedMultiplier = cSieve->mpzFixedMultiplier;
			//printf("[%02d] debugStats_multipliersTested: %d\n", cSieve->currentSieveLayerIdx, debugStats_multipliersTested);
			//printf("[%02d] debugStats_primes: %d\n", cSieve->currentSieveLayerIdx, debugStats_primes);
			//double primality = (double)debugStats_primes / (double)debugStats_multipliersTested;
			//printf("[%02d] debugStats_primality: %lf\n", cSieve->currentSieveLayerIdx, primality);
			debugStats_primes = 0;
			debugStats_multipliersTested = 0;
			continue;
		}
		 
		//// test for sieve bugs C1
		//if( (sieveFlags&SIEVE_FLAG_C1_COMPOSITE)==0 && (rand()%10)==0 )
		//{
		//	// test c1
		//	for(uint32 lt=0; lt<cSieve->chainLength; lt++)
		//	{
		//		uint32 aMult = 1<<lt;
		//		mpzChainOrigin = mpzFinalFixedMultiplier * multiplier * aMult - 1;	
		//		for(uint32 f=0; f<cSieve->numPrimeFactors; f++)
		//		{
		//			uint32 mod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzChainOrigin.get_mpz_t(), vPrimes[f]);
		//			if( mod == 0 )
		//				printf("c1 div by %d possible\n", vPrimes[f]);//__debugbreak();
		//		}
		//	}
		//}
		//// test for sieve bugs C2
		//if( (sieveFlags&SIEVE_FLAG_C2_COMPOSITE)==0 && (rand()%10)==0 )
		//{
		//	// test c1
		//	for(uint32 lt=0; lt<cSieve->chainLength; lt++)
		//	{
		//		uint32 aMult = 1<<lt;
		//		mpzChainOrigin = mpzFinalFixedMultiplier * multiplier * aMult + 1;	
		//		for(uint32 f=0; f<cSieve->numPrimeFactors; f++)
		//		{
		//			uint32 mod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzChainOrigin.get_mpz_t(), vPrimes[f]);
		//			if( mod == 0 )
		//				printf("c2 div by %d possible\n", vPrimes[f]);//__debugbreak();
		//		}
		//	}
		//}
		// test for sieve bugs BT
		//if( (sieveFlags&SIEVE_FLAG_BT_COMPOSITE)==0 )
		//{
		//	// test c1
		//	mpzChainOrigin = mpzFinalFixedMultiplier * multiplier + 1;	
		//	for(uint32 f=0; f<cSieve->numPrimeFactors; f++)
		//	{
		//		uint32 mod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzChainOrigin.get_mpz_t(), vPrimes[f]);
		//		if( mod == 0 )
		//			printf("bt-c2 div by %d possible\n", vPrimes[f]);
		//	}
		//	// test c2
		//	mpzChainOrigin = mpzFinalFixedMultiplier * multiplier - 1;	
		//	for(uint32 f=0; f<cSieve->numPrimeFactors; f++)
		//	{
		//		uint32 mod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzChainOrigin.get_mpz_t(), vPrimes[f]);
		//		if( mod == 0 )
		//			printf("bt-c2 div by %d possible\n", vPrimes[f]);
		//	}
		//}

		//mpzChainOrigin = mpzFinalFixedMultiplier * multiplier;		
		mpz_mul_ui(mpzChainOrigin.get_mpz_t(), mpzFinalFixedMultiplier.get_mpz_t(), multiplier);
		nChainLengthCunningham1 = 0;
		nChainLengthCunningham2 = 0;
		nChainLengthBiTwin = 0;

		bool canSubmitAsShare = ProbablePrimeChainTestFast2(mpzChainOrigin, testParams, sieveFlags, multiplier);
		nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);

		if( nProbableChainLength >= 0x01000000 )
			debugStats_primes++;
		debugStats_multipliersTested++;
		//bool canSubmitAsShare = ProbablePrimeChainTestFast(mpzChainOrigin, testParams);
		//CBigNum bnChainOrigin;
		//bnChainOrigin.SetHex(mpzChainOrigin.get_str(16));
		//bool canSubmitAsShare = ProbablePrimeChainTestBN(bnChainOrigin, block->serverData.nBitsForShare, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin);
		


		if( nProbableChainLength >= 0x04000000 )
		{
			sint32 chainDif = (nProbableChainLength>>24) - 7;
			primeStats.nChainHit += pow(8, (float)chainDif);
			//primeStats.nChainHit += pow(8, ((float)((double)nProbableChainLength  / (double)0x1000000))-7.0);
			//primeStats.nChainHit += pow(8, floor((float)((double)nProbableChainLength  / (double)0x1000000)) - 7);
			nTests = 0;
			primeStats.fourChainCount ++;
			if (nProbableChainLength >= 0x5000000)
			{
				primeStats.fiveChainCount ++;
				if (nProbableChainLength >= 0x6000000)
				{
					primeStats.sixChainCount ++;
					if (nProbableChainLength >= 0x7000000)
						primeStats.sevenChainCount ++;
				}
			}
		}
		//if( nBitsGen >= 0x03000000 )
		//	printf("%08X\n", nBitsGen);
		primeStats.primeChainsFound++;
		//if( nProbableChainLength > 0x03000000 )
		//	primeStats.qualityPrimesFound++;
		if( nProbableChainLength > primeStats.bestPrimeChainDifficulty )
			primeStats.bestPrimeChainDifficulty = nProbableChainLength;
		
		if(nProbableChainLength >= block->serverData.nBitsForShare)
		{
			// note: mpzPrimeChainMultiplier does not include the blockHash multiplier
			mpz_div(block->mpzPrimeChainMultiplier.get_mpz_t(), mpzChainOrigin.get_mpz_t(), mpzHash.get_mpz_t());
			//mpz_lsh(block->mpzPrimeChainMultiplier.get_mpz_t(), mpzFixedMultiplier.get_mpz_t(), multiplier);
			// update server data
			block->serverData.client_shareBits = nProbableChainLength;
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

			printf("%s - SHARE FOUND !!! (Th#: %u Multiplier: %d Layer: %d) ---  DIFF: %f    %s    %s\n", 
				sNow, threadIndex, multiplier, cSieve->currentSieveLayerIdx, (float)((double)nProbableChainLength  / (double)0x1000000), 
				nProbableChainLength >= 0x6000000 ? ">6":"", nProbableChainLength >= 0x7000000 ? ">7":"");

			// submit this share
			if (jhMiner_pushShare_primecoin(blockRawData, block))
				primeStats.foundShareCount ++;
			//printf("Probable prime chain found for block=%s!!\n  Target: %s\n  Length: (%s %s %s)\n", block.GetHash().GetHex().c_str(),TargetToString(block.nBits).c_str(), TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
			//nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);
			// since we are using C structs here we have to make sure the memory for the CBigNum in the block is freed again
			//delete *psieve;
			//*psieve = NULL;
			//block->bnPrimeChainMultiplier = NULL;
			RtlZeroMemory(blockRawData, 256);
			//delete *psieve;
			//*psieve = NULL;
			// dont quit if we find a share, there could be other shares in the remaining prime candidates
			nTests = 0;   // tehere is a good chance to find more shares so search a litle more.
			//block->nonce++;
			//return true;
			//break;
			//if (multipleShare)
		}
	}
	//__debugbreak();
	return false;
}


void BitcoinMiner2(primecoinBlock_t* primecoinBlock, sint32 threadIndex)
{
	if( pctx == NULL )
		pctx = BN_CTX_new();
	unsigned int nExtraNonce = 0;

	static const unsigned int nPrimorialHashFactor = 7;
	const unsigned int nPrimorialMultiplierStart = 61;   
	const unsigned int nPrimorialMultiplierMax = 79;

	unsigned int nPrimorialMultiplier = primeStats.nPrimorialMultiplier;
	int64 nTimeExpected = 0;   // time expected to prime chain (micro-second)
	int64 nTimeExpectedPrev = 0; // time expected to prime chain last time
	bool fIncrementPrimorial = true; // increase or decrease primorial factor
	int64 nSieveGenTime = 0;


	CSieveOfEratosthenes* psieve = NULL;

	if( primecoinBlock->xptMode )
		primecoinBlock->nonce = 0; // xpt guarantees unique merkleRoot hashes for each thread
	else
		primecoinBlock->nonce = 0x1000 * primecoinBlock->threadIndex;
	uint32 nStatTime = GetTickCount() + 2000;

	// note: originally a wanted to loop as long as (primecoinBlock->workDataHash != jhMiner_getCurrentWorkHash()) did not happen
	//		 but I noticed it might be smarter to just check if the blockHeight has changed, since that is what is really important
	uint32 loopCount = 0;

	//mpz_class mpzHashFactor;
	//Primorial(nPrimorialHashFactor, mpzHashFactor);
	unsigned int nHashFactor = PrimorialFast(nPrimorialHashFactor);

	time_t unixTimeStart;
	time(&unixTimeStart);
	uint32 nTimeRollStart = primecoinBlock->timestamp;

	uint32 nCurrentTick = GetTickCount();
	while( primecoinBlock->serverData.blockHeight == jhMiner_getCurrentWorkBlockHeight(primecoinBlock->threadIndex) )
	{
		nCurrentTick = GetTickCount();
		//if( primecoinBlock->xptMode )
		//{
		//	// when using x.pushthrough, roll time
		//	time_t unixTimeCurrent;
		//	time(&unixTimeCurrent);
		//	uint32 timeDif = unixTimeCurrent - unixTimeStart;
		//	uint32 newTimestamp = nTimeRollStart + timeDif;
		//	if( newTimestamp != primecoinBlock->timestamp )
		//	{
		//		primecoinBlock->timestamp = newTimestamp;
		//		primecoinBlock->nonce = 0;
		//		//nPrimorialMultiplierStart = startFactorList[(threadIndex&3)];
		//		nPrimorialMultiplier = nPrimorialMultiplierStart;
		//	}
		//}

		primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
		//
		// Search
		//
		bool fNewBlock = true;
		unsigned int nTriedMultiplier = 0;
		// Primecoin: try to find hash divisible by primorial
		uint256 phash = primecoinBlock->blockHeaderHash;
		mpz_class mpzHash;
		mpz_set_uint256(mpzHash.get_mpz_t(), phash);

		while ((phash < hashBlockHeaderLimit || !mpz_divisible_ui_p(mpzHash.get_mpz_t(), nHashFactor)) && primecoinBlock->nonce < 0xffff0000) {
			primecoinBlock->nonce++;
			primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
			phash = primecoinBlock->blockHeaderHash;
			mpz_set_uint256(mpzHash.get_mpz_t(), phash);
		}
		//printf("Use nonce %d\n", primecoinBlock->nonce);
		if (primecoinBlock->nonce >= 0xffff0000)
		{
			printf("Nonce overflow\n");
			break;
		}
		// Primecoin: primorial fixed multiplier
		mpz_class mpzPrimorial;
		unsigned int nRoundTests = 0;
		unsigned int nRoundPrimesHit = 0;
		int64 nPrimeTimerStart = GetTickCount();

		//if( loopCount > 0 )
		//{
		//	//primecoinBlock->nonce++;
		//	if (!PrimeTableGetNextPrime(nPrimorialMultiplier))
		//		error("PrimecoinMiner() : primorial increment overflow");
		//}

		Primorial(nPrimorialMultiplier, mpzPrimorial);

		unsigned int nTests = 0;
		unsigned int nPrimesHit = 0;

		mpz_class mpzMultiplierMin = mpzPrimeMin * nHashFactor / mpzHash + 1;
		while (mpzPrimorial < mpzMultiplierMin )
		{
			if (!PrimeTableGetNextPrime(nPrimorialMultiplier))
				error("PrimecoinMiner() : primorial minimum overflow");
			Primorial(nPrimorialMultiplier, mpzPrimorial);
		}
		mpz_class mpzFixedMultiplier = mpzPrimorial;
		/*if (mpzPrimorial > nHashFactor) {
			mpzFixedMultiplier = mpzPrimorial / nHashFactor;
		} else {
			mpzFixedMultiplier = 1;
		}		*/


		//printf("fixedMultiplier: %d nPrimorialMultiplier: %d\n", BN_get_word(&bnFixedMultiplier), nPrimorialMultiplier);
		// Primecoin: mine for prime chain
		unsigned int nProbableChainLength;
		BitcoinMiner2_mineProbableChain(primecoinBlock, mpzFixedMultiplier, fNewBlock, nTriedMultiplier, nProbableChainLength, nTests, nPrimesHit, threadIndex, mpzHash, nPrimorialMultiplier);
		//psieve = NULL;
		nRoundTests += nTests;
		nRoundPrimesHit += nPrimesHit;
		nPrimorialMultiplier = primeStats.nPrimorialMultiplier;
		// added this
		if (fNewBlock)
		{
		}


		primecoinBlock->nonce++;
		primecoinBlock->timestamp = max(primecoinBlock->timestamp, (unsigned int) time(NULL));
		loopCount++;
	}

}