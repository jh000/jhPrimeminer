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
	//uint32			numPrimeFactors; // how many prime factors are filtered
	cPrimeFactor_t*	primeFactorBase;
	uint32			chainLength; // equal to number of layers
	uint8*			layerMaskC1;
	uint8*			layerMaskC2;
	//uint32**		layerMaskBT;
	uint32			maskBytes;
	// multiplier scan
	uint32			currentMultiplierScanIndex;
	uint32			currentSieveLayerIdx;
	// mpz
	mpz_class		mpzFixedMultiplier;
	// settings used to create the sieve
	uint32			nPrimeFactors;
	uint32			sieveSize;
}continuousSieve_t;

__declspec( thread ) continuousSieve_t* cSieve = NULL;

void cSieve_prepare(mpz_class& mpzFixedMultiplier, uint32 chainLength, uint32 overwritePrimeFactors=0, uint32 overwriteSieveSize=0)
{
	uint32 sieveSize = (minerSettings.nSieveSize+31)&~31; // align to 32 (4 bytes)
	if( overwriteSieveSize )
		sieveSize = (overwriteSieveSize+31)&~31;
	// todo: Make sure the sieve doesnt crash when the difficulty changes
	if( cSieve == NULL || cSieve->chainLength != chainLength || cSieve->sieveSize != sieveSize || cSieve->nPrimeFactors != minerSettings.nPrimesToSieve )
	{
		if( cSieve == NULL )
		{
			cSieve = (continuousSieve_t*)malloc(sizeof(continuousSieve_t));
			memset(cSieve, 0x00, sizeof(continuousSieve_t));
		}
		// start setting up sieve properties early (to avoid threading issues)
		cSieve->sieveSize = sieveSize;
		cSieve->chainLength = chainLength;
		if( cSieve->sieveSize & 7 )
			__debugbreak(); // must not happen
		cSieve->maskBytes = cSieve->sieveSize/8;
		if( overwritePrimeFactors == 0 )
			cSieve->nPrimeFactors = minerSettings.nPrimesToSieve;
		else
			cSieve->nPrimeFactors = overwritePrimeFactors;
		// free everything that has been allocated before
		if( cSieve->layerMaskC1 )
		{
			free(cSieve->layerMaskC1);
			free(cSieve->layerMaskC2);
			cSieve->layerMaskC1 = NULL;
			cSieve->layerMaskC2 = NULL;
		}
		if( cSieve->primeFactorBase )
		{
			free(cSieve->primeFactorBase);
			cSieve->primeFactorBase = NULL;
		}
		// alloc sieve mask
		uint32 chainMaskSize = cSieve->maskBytes * chainLength;
		if( cSieve->layerMaskC1 == NULL )
		{
			cSieve->layerMaskC1 = (uint8*)malloc(chainMaskSize);
			cSieve->layerMaskC2 = (uint8*)malloc(chainMaskSize);
		}
		// alloc prime factor base table
		if( cSieve->primeFactorBase == NULL )
		{
			cSieve->primeFactorBase = (cPrimeFactor_t*)malloc(sizeof(cPrimeFactor_t)*cSieve->nPrimeFactors);
			for(uint32 i=0; i<cSieve->nPrimeFactors; i++)
			{
				cSieve->primeFactorBase[i].twoInverse = single_modinv(2, vPrimes[i]);
			}
		}
	}
	// calculate fixed inverse
	mpz_class mpzF, mpzTemp;
	for(uint32 i=0; i<cSieve->nPrimeFactors; i++)
	{
		uint32 p = vPrimes[i];
		uint32 fixedFactorMod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzFixedMultiplier.get_mpz_t(), p);
		cSieve->primeFactorBase[i].mod = fixedFactorMod; 
		cSieve->primeFactorBase[i].base = single_modinv(fixedFactorMod, p); 
	}
	// reset sieve
	memset(cSieve->layerMaskC1, 0x00, cSieve->maskBytes * chainLength);
	memset(cSieve->layerMaskC2, 0x00, cSieve->maskBytes * chainLength);
	cSieve->mpzFixedMultiplier = mpzFixedMultiplier;
	for(uint32 t=0; t<cSieve->chainLength; t++)
	{
		mpzF = mpzFixedMultiplier;
		uint8* maskC1 = cSieve->layerMaskC1+t*cSieve->maskBytes; 
		uint8* maskC2 = cSieve->layerMaskC2+t*cSieve->maskBytes; 
		for(uint32 i=0; i<cSieve->nPrimeFactors; i++)
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
			nSolvedMultiplier = -cSieve->primeFactorBase[i].base + p; // tested and correct
			nSolvedMultiplier %= (uint64)p;
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
	if( cSieve->currentSieveLayerIdx > 108 )
		return false;
	uint32 layerIdx = cSieve->currentSieveLayerIdx % cSieve->chainLength;
	uint8* maskC1 = cSieve->layerMaskC1+layerIdx*cSieve->maskBytes; 
	uint8* maskC2 = cSieve->layerMaskC2+layerIdx*cSieve->maskBytes;
	memset(maskC1, 0x00, cSieve->maskBytes);
	memset(maskC2, 0x00, cSieve->maskBytes);
	for(uint32 i=0; i<cSieve->nPrimeFactors; i++)
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
		nSolvedMultiplier = p - cSieve->primeFactorBase[i].base;
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
	if( cSieve->currentSieveLayerIdx > 0 )
		multiplier |= 1; // make sure we only process odd multipliers on every layer above 0
	if( cSieve->chainLength >= 128 )
	{
		printf("cSieve_findNextMultiplier(): fatal error, difficulty exceeds allowed value\n");
		ExitProcess(0);
	}
	uint32* maskPtrC1[32]; // works unless difficulty is greater than 32
	uint32* maskPtrC2[32];
	for(uint32 i=0; i<cSieve->chainLength; i++)
	{
		maskPtrC1[i] = (uint32*)(cSieve->layerMaskC1+i*cSieve->maskBytes);
		maskPtrC2[i] = (uint32*)(cSieve->layerMaskC2+i*cSieve->maskBytes);
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
		uint32 compositeMaskIdx = multiplier>>5;
		// bitwin mask part
		uint32 maskBtC1 = 0;
		uint32 maskBtC2 = 0;
		for(uint32 i=0; i<btn; i++)
		{
			maskBtC1 |= maskPtrC1[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
			maskBtC2 |= maskPtrC2[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
			// note: We ignore the fact that we should actually look for 9 length, not 8 due to 9/2 = 4 (will fix itself when difficulty is even)
		}
		// remaining mask
		uint32 maskC1 = 0;
		uint32 maskC2 = 0;
		for(uint32 i=btn; i<cSieve->chainLength; i++)
		{
			maskC1 |= maskPtrC1[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
			maskC2 |= maskPtrC2[(sieveLayerIdx+i)%cSieve->chainLength][compositeMaskIdx];
		}
		maskC1 |= maskBtC1;
		maskC2 |= maskBtC2;
		uint32 maskBt = maskBtC1|maskBtC2;
		// should we skip parsing this mask?
		if( maskC1 == 0xFFFFFFFF && maskC2 == 0xFFFFFFFF && maskBt == 0xFFFFFFFF )
		{
			multiplier = (multiplier+31)&~31;
			multiplier = multiplier + 32;
			continue;
		}
		// parse the mask bits
		if( cSieve->currentSieveLayerIdx > 0 )
			multiplier |= 1;
		uint32 multParse = multiplier - (multiplier&~31);
		multParse = 32 - multParse;
		if( cSieve->currentSieveLayerIdx > 0 )
		{
			// parse odd 32 bit
			for(uint32 t=0; t<multParse; t += 2)
			{
				// process until aligned to 32
				if( multiplier >= cSieve->sieveSize )
				{
					cSieve->currentMultiplierScanIndex = cSieve->sieveSize;
					return 0;
				}
				uint32 compositeMask = 1<<(multiplier&31);
				//printf("%08X\n", 1<<(multiplier&31));

				if( (maskBt&maskC1&maskC2&compositeMask)==0 )
				{
					// good multiplier found
					if( maskBt&compositeMask )
						*multiplierSieveFlags |= SIEVE_FLAG_BT_COMPOSITE;
					if( maskC1&compositeMask )
						*multiplierSieveFlags |= SIEVE_FLAG_C1_COMPOSITE;
					if( maskC2&compositeMask )
						*multiplierSieveFlags |= SIEVE_FLAG_C2_COMPOSITE;
					cSieve->currentMultiplierScanIndex = multiplier+2;
					return multiplier;
				}
				multiplier += 2;
			}
		}
		else
		{
			// parse full 32 bit
			for(uint32 t=0; t<multParse; t++)
			{
				// process until aligned to 32
				if( multiplier >= cSieve->sieveSize )
				{
					cSieve->currentMultiplierScanIndex = cSieve->sieveSize;
					return 0;
				}
				uint32 compositeMask = 1<<(multiplier&31);

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
				multiplier++;
			}
		}	
	}
	return 0;
}

/*
 * Used to print out the first 100 multipliers that passed the sieve
 * Useful to compare with other sieve implementations
 */
void cSieve_debug(mpz_class& mpzFixedMultiplier, uint32 chainLength, uint32 primeFactors, uint32 sieveSize)
{
	cSieve_prepare(mpzFixedMultiplier, chainLength, primeFactors, sieveSize);
	
	uint32 rows = 128;
	uint32 pixelsPerRow = cSieve->sieveSize / rows;

	uint8* pixelMap = (uint8*)malloc(cSieve->sieveSize*(cSieve->chainLength*2)*3);
	memset(pixelMap, 0x00, cSieve->sieveSize*cSieve->chainLength*2*3);
	uint32 pixelPerRow = cSieve->sieveSize;
	for(sint32 y=0; y<cSieve->chainLength; y++)
	{
		for(sint32 x=0; x<cSieve->sieveSize; x++)
		{
			uint32 rowY = y*cSieve->chainLength*2 + x / pixelsPerRow;
			uint32 maskBase = cSieve->maskBytes*rowY;
			uint32 maskIdx = x>>3;
			uint32 maskVal = 1<<(x&7);
			uint32 mC1 = cSieve->layerMaskC1[maskBase+maskIdx] & maskVal;
			uint32 mC2 = cSieve->layerMaskC2[maskBase+maskIdx] & maskVal;
			uint32 pIdx = (x+y*pixelsPerRow)*3;
			pixelMap[pIdx+0] = 0;
			pixelMap[pIdx+1] = 0;
			pixelMap[pIdx+2] = 0;
			if( mC1 == 0 )
				pixelMap[pIdx+2] = 0xFF;
			if( mC2 == 0 )
				pixelMap[pIdx+0] = 0xFF;
		}
	}	
	bitmap_t bmp;
	bmp.bitDepth = 24;
	bmp.data = pixelMap;
	bmp.sizeX = pixelsPerRow;
	bmp.sizeY = cSieve->chainLength*rows;
	printf("cSieve_debug(): Saving sieve debug image...\n");
	bmp_save("C:\\test\\sieve.bmp", &bmp);

	uint32 multiplier = 0;
	for(uint32 i=0; i<100; i++)
	{
		uint32 sieveFlags = 0;
		multiplier = cSieve_findNextMultiplier(&sieveFlags);
		if( multiplier == 0 )
			break;
		printf("cSieve mult[%04d]: %d\n", i, multiplier);
	}
}

bool ProbableCunninghamChainTestFast(const mpz_class& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength, CPrimalityTestParams& testParams, bool doTrialDivision);

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

	//sieveFlags = 0; // note: enabling the sieve optimization seems to decrease the rate of shares (remove this line to enable this future for non-gpu mode)

	bool testCFirstKind = ((sieveFlags&SIEVE_FLAG_BT_COMPOSITE)==0) || ((sieveFlags&SIEVE_FLAG_C1_COMPOSITE)==0); // yes, C1 and C2 are switched
	bool testCSecondKind = ((sieveFlags&SIEVE_FLAG_BT_COMPOSITE)==0) || ((sieveFlags&SIEVE_FLAG_C2_COMPOSITE)==0);

	bool testCFirstFast = (sieveFlags&SIEVE_FLAG_C1_COMPOSITE)!=0; // should be !=0?
	bool testCSecondFast = (sieveFlags&SIEVE_FLAG_C2_COMPOSITE)!=0;

	mpzOriginMinusOne = mpzPrimeChainOrigin - 1; // first kind origin
	mpzOriginPlusOne = mpzPrimeChainOrigin + 1; // second kind origin

	// detailed test

	//if( testCFirstKind && testCFirstFast == true ) //(sieveFlags&SIEVE_FLAG_C1_COMPOSITE)==0 )
	//{
	//	for(uint32 i=0; i<5000; i++)
	//	{
	//		if( mpz_tdiv_ui(mpzOriginMinusOne.get_mpz_t(), vPrimes[i]) == 0 )
	//			__debugbreak();
	//	}
	//}
	//if( testCSecondKind && testCSecondFast == true )//(sieveFlags&SIEVE_FLAG_C2_COMPOSITE)==0 )
	//{
	//	for(uint32 i=0; i<5000; i++)
	//	{
	//		if( mpz_tdiv_ui(mpzOriginPlusOne.get_mpz_t(), vPrimes[i]) == 0 )
	//			__debugbreak();
	//	}
	//}


	//printf("%d\n", fFermatTest?1:0);

	// Test for Cunningham Chain of first kind
	if( testCFirstKind )
	{
		ProbableCunninghamChainTestFast(mpzOriginMinusOne, true, fFermatTest, nChainLengthCunningham1, testParams, testCFirstFast);
	
		//if( nChainLengthCunningham1 >= 0x04000000 )
		//{
		//	//__debugbreak();
		//	mpz_class mpz_r;
		//	mpz_class mpz_n1 = mpzPrimeChainOrigin - 1;
		//	mpz_class mpz_n2 = mpzPrimeChainOrigin*2 - 1;
		//	mpz_class mpz_n3 = mpzPrimeChainOrigin*4 - 1;
		//	mpz_class mpz_n4 = mpzPrimeChainOrigin*8 - 1;
		//	mpz_class mpz_e = mpz_n3 - 1;
		//	mpz_class mpz_m = mpz_n1 * mpz_n2 * mpz_n3 * mpz_n4;
		//	mpz_powm(mpz_r.get_mpz_t(), mpzTwo.get_mpz_t(), mpz_e.get_mpz_t(), mpz_m.get_mpz_t());

		//	bool isPrime = (mpz_r.get_mpz_t()->_mp_d[0]&0xFF)==0xFF;
		//	printf("%08X isPrime: %s	D: %016X\n", nChainLengthCunningham1, isPrime?"yes":"no", mpz_r.get_mpz_t()->_mp_d[0]);
		//	if( isPrime == false )
		//		__debugbreak();
		//	//__debugbreak();


		//}
	}
	else
		nChainLengthCunningham1 = 0;
	// Test for Cunningham Chain of second kind
	if( testCSecondKind )
		ProbableCunninghamChainTestFast(mpzOriginPlusOne, false, fFermatTest, nChainLengthCunningham2, testParams, testCSecondFast);
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
		//unsigned long lDivisor = 1;
		//unsigned int i;
		//testParams.vFastDivSeq.push_back(nPrimorialSeq);
		//unsigned long primeIdx = nPrimorialSeq;
		//unsigned long pStart = primeIdx;
		//for (i = 1; i <= nFastDivPrimes; i++)
		//{
		//	// Multiply primes together until the result won't fit an unsigned long
		//	if (lDivisor < ULONG_MAX / vPrimes[primeIdx])
		//	{
		//		lDivisor *= vPrimes[primeIdx];
		//		primeIdx++;
		//	}
		//	else
		//	{
		//		// cannot multiply anymore
		//		testParams.vFastDivisors.push_back(lDivisor);
		//		testParams.vFastDivSeq.push_back(primeIdx);
		//		lDivisor = 1;
		//		pStart = primeIdx;
		//	}
		//	testParams.nFastDivisorsSize = testParams.vFastDivSeq.size() - 1;
		//}

		// Finish off by multiplying as many primes as possible
		/*while (lDivisor < ULONG_MAX / vPrimes[nPrimorialSeq + i])
		{
			lDivisor *= vPrimes[nPrimorialSeq + i];
			i++;
		}
		testParams.vFastDivisors.push_back(lDivisor);
		testParams.vFastDivSeq.push_back(nPrimorialSeq + i);
		testParams.nFastDivisorsSize = testParams.vFastDivisors.size();*/
	}
	// References to counters;
	unsigned int& nChainLengthCunningham1 = testParams.nChainLengthCunningham1;
	unsigned int& nChainLengthCunningham2 = testParams.nChainLengthCunningham2;
	unsigned int& nChainLengthBiTwin = testParams.nChainLengthBiTwin;

	while( block->blockHeight == jhMiner_getCurrentWorkBlockHeight(block->threadIndex) )
	{
		uint32 sieveFlags;
		uint32 multiplier = cSieve_findNextMultiplier(&sieveFlags);
		if( multiplier == 0 )
		{
			// mix in next layer
			if( cSieve_sieveNextLayer() == false )
				break;
			mpzFinalFixedMultiplier = cSieve->mpzFixedMultiplier;
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
			primeStats.numPrimeCandidates++;
		primeStats.numTestedCandidates++;
		//debugStats_multipliersTested++;
		//bool canSubmitAsShare = ProbablePrimeChainTestFast(mpzChainOrigin, testParams);
		//CBigNum bnChainOrigin;
		//bnChainOrigin.SetHex(mpzChainOrigin.get_str(16));
		//bool canSubmitAsShare = ProbablePrimeChainTestBN(bnChainOrigin, block->serverData.nBitsForShare, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin);
		


		if( nProbableChainLength >= 0x04000000 )
		{
			sint32 chainDif = (nProbableChainLength>>24) - 4;
			uint64 chainDifInt = (uint64)pow(10.0, (double)chainDif);
			primeStats.nChainHit += chainDifInt;
			//primeStats.nChainHit += pow(8, ((float)((double)nProbableChainLength  / (double)0x1000000))-7.0);
			//primeStats.nChainHit += pow(8, floor((float)((double)nProbableChainLength  / (double)0x1000000)) - 7);
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
		//	printf("%08X\n", nBitsGen);
		primeStats.primeChainsFound++;
		//if( nProbableChainLength > 0x03000000 )
		//	primeStats.qualityPrimesFound++;
		if( nProbableChainLength > primeStats.bestPrimeChainDifficulty )
			primeStats.bestPrimeChainDifficulty = nProbableChainLength;
		
		if(nProbableChainLength >= block->nBitsForShare)
		{
			// note: mpzPrimeChainMultiplier does not include the blockHash multiplier
			mpz_div(block->mpzPrimeChainMultiplier.get_mpz_t(), mpzChainOrigin.get_mpz_t(), mpzHash.get_mpz_t());
			//mpz_lsh(block->mpzPrimeChainMultiplier.get_mpz_t(), mpzFixedMultiplier.get_mpz_t(), multiplier);
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
			RtlZeroMemory(blockRawData, 256);
			// dont quit if we find a share, there could be other shares in the remaining prime candidates
			nTests = 0;   // tehere is a good chance to find more shares so search a litle more.
		}
	}
	//__debugbreak();
	return false;
}

void BitcoinMiner_multipassSieve(primecoinBlock_t* primecoinBlock, sint32 threadIndex)
{
	if( pctx == NULL )
		pctx = BN_CTX_new();
	unsigned int nExtraNonce = 0;

	static unsigned int nPrimorialHashFactor = primecoinBlock->fixedHashFactor;//7 11;
	unsigned int nPrimorialMultiplier = primecoinBlock->fixedPrimorial;
	int64 nTimeExpected = 0;   // time expected to prime chain (micro-second)
	int64 nTimeExpectedPrev = 0; // time expected to prime chain last time
	bool fIncrementPrimorial = true; // increase or decrease primorial factor
	int64 nSieveGenTime = 0;

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
	primecoinBlock->nonce = primecoinBlock->nonceMin;
	while( primecoinBlock->blockHeight == jhMiner_getCurrentWorkBlockHeight(primecoinBlock->threadIndex) )
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

		while ((phash < hashBlockHeaderLimit || !mpz_divisible_ui_p(mpzHash.get_mpz_t(), nHashFactor)) && primecoinBlock->nonce < 0xf0000000) {
			primecoinBlock->nonce++;
			primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
			phash = primecoinBlock->blockHeaderHash;
			mpz_set_uint256(mpzHash.get_mpz_t(), phash);
		}
		//printf("Use nonce %d\n", primecoinBlock->nonce);
		if (primecoinBlock->nonce >= 0xf0000000)
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
		//if( mpz_divisible_ui_p(mpzPrimorial.get_mpz_t(), nHashFactor) )
		//	__debugbreak();
		mpz_class mpzFixedMultiplier = mpzPrimorial / nHashFactor;
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
		// added this
		if (fNewBlock)
		{
		}
		

		primecoinBlock->nonce++;
		primecoinBlock->timestamp = max(primecoinBlock->timestamp, (unsigned int) time(NULL));
		loopCount++;
	}

}