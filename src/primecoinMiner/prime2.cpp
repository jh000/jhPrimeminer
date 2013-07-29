// Copyright (c) 2013 Primecoin developers
// Distributed under conditional MIT/X11 software license,
// see the accompanying file COPYING

#include "global.h"
/* removed dep for git until fully working.
#include <cuda.h>
#include <cuda_runtime_api.h>

#include "cuda\mainkernel.h"


//check for cuda error and initilize
void checkForCudaError(const char* desc)
{
    cudaError_t errCode = cudaGetLastError();
    if (errCode != cudaSuccess)
    {
        printf("Cuda error: %s, %s\n",desc,cudaGetErrorString(errCode));
        exit(0);
    }
}

//template function for array allocation on the device
template <typename T>
T *newDevicePointer(size_t size)
{
    T *devicePointer;
    cudaMalloc((void **) &devicePointer, sizeof(T)*size);
    checkForCudaError("cudaMalloc");
    return devicePointer;
}


int initializeCUDA(int device)
{
    cudaSetDevice(device);
	printf("Init Cuda Device: %d\n",device);
    return 0;
}
*/
// Prime Table
//std::vector<unsigned int> vPrimes;
uint32* vPrimes;
uint32* vPrimesTwoInverse;
uint32 vPrimesSize = 0;

//std::vector<unsigned int> vPrimes;
std::vector<unsigned int> vTwoInverses;

__declspec( thread ) BN_CTX* pctx = NULL;

// changed to return the ticks since reboot
// doesnt need to be the correct time, just a more or less random input value
uint64 GetTimeMicros()
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (uint64)t.QuadPart;
}

//void GeneratePrimeTable()
//{
//	static const unsigned int nPrimeTableLimit = nMaxSieveSize;
//	vPrimes.clear();
//	// Generate prime table using sieve of Eratosthenes
//	std::vector<bool> vfComposite (nPrimeTableLimit, false);
//	for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
//	{
//		if (vfComposite[nFactor])
//			continue;
//		for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
//			vfComposite[nComposite] = true;
//	}
//	for (unsigned int n = 2; n < nPrimeTableLimit; n++)
//		if (!vfComposite[n])
//			vPrimes.push_back(n);
//	printf("GeneratePrimeTable() : prime table [1, %d] generated with %lu primes", nPrimeTableLimit, vPrimes.size());
//	//BOOST_FOREACH(unsigned int nPrime, vPrimes)
//	//    printf(" %u", nPrime);
//	printf("\n");
//}

int isqrt(long n) {
    register int i;
    register long root=0, left=0;
    for (i = (sizeof(long)<<3) - 2; i >= 0; i -= 2)
    {
        left = left<<2 | n>>i & 3 ;
        root <<= 1;
        if ( left > root )
            left -= ++root, ++root;
    }
    return (int)(root>>1);
}

void GeneratePrimeTable()
{
	static const unsigned int nPrimeTableLimit = commandlineInput.sievePrimeLimit;
	//vPrimes.clear();
	vPrimesSize = 0;
	vPrimes = (uint32*)malloc(sizeof(uint32)*nPrimeTableLimit);
	// Generate prime table using sieve of Atkin
	uint8* vfComposite = (uint8*)malloc(nPrimeTableLimit);
	RtlZeroMemory(vfComposite, nPrimeTableLimit);
	for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
	{
		if (vfComposite[nFactor])
			continue;
		for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
			vfComposite[nComposite] = true;
	}
	for (unsigned int n = 2; n < nPrimeTableLimit; n++)
	{
		if (!vfComposite[n])
		{
			vPrimes[vPrimesSize] = n;
			vPrimesSize++;
		}
	}
	printf("GeneratePrimeTable() : prime table [1, %d] generated with %lu primes\n", nPrimeTableLimit, vPrimesSize);
	free(vfComposite);
	vPrimes = (uint32*)realloc(vPrimes, sizeof(uint32)*vPrimesSize);
	// calculate two's inverse for all primes
	vPrimesTwoInverse = (uint32*)malloc(sizeof(uint32)*vPrimesSize);
	CBigNum bnTwo = 2;
	BIGNUM bn_twoInverse = {0};
	BN_init(&bn_twoInverse);
	BIGNUM bn_p = {0};
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		BN_set_word(&bn_p, vPrimes[i]);
		if (!BN_mod_inverse(&bn_twoInverse, &bnTwo, &bn_p, pctx))
		{
			BN_set_word(&bn_twoInverse, 0);
		}
		vPrimesTwoInverse[i] = (uint32)BN_get_word(&bn_twoInverse);
	}
}

// Get next prime number of p
bool PrimeTableGetNextPrime(unsigned int* p)
{
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		if (vPrimes[i] > *p)
		{
			*p = vPrimes[i];
			return true;
		}
	}
	return false;
}

// Get previous prime number of p
bool PrimeTableGetPreviousPrime(unsigned int* p)
{
	uint32 nPrevPrime = 0;
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		if (vPrimes[i] >= *p)
			break;
		nPrevPrime = vPrimes[i];
	}
	if (nPrevPrime)
	{
		*p = nPrevPrime;
		return true;
	}
	return false;
}

// Compute Primorial number p#
void Primorial(unsigned int p, mpz_class& bnPrimorial)
{
	bnPrimorial = 1;
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];
		if (nPrime > p) break;
		bnPrimorial *= nPrime;
	}
}

// Compute first primorial number greater than or equal to pn
void PrimorialAt(mpz_class& bn, mpz_class& bnPrimorial)
{
	bnPrimorial = 1;
	//BOOST_FOREACH(unsigned int nPrime, vPrimes)
	//for(uint32 i=0; i<vPrimes.size(); i++)
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];
		bnPrimorial *= nPrime;
		if (bnPrimorial >= bn)
			return;
	}
}

// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTest(const mpz_class& n, unsigned int& nLength)
{
	/*//mpz_class a = 2; // base; Fermat witness
	mpz_class e = n - 1;
	mpz_class r;
	BN_mod_exp(&r, &bnTwo, &e, &n, pctx);
	if (r == 1)
		return true;
	// Failed Fermat test, calculate fractional length
	unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
	if (nFractionalLength >= (1 << nFractionalBits))
		return error("FermatProbablePrimalityTest() : fractional assert");
	nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
	return false;*/
	 // Faster GMP version
    
    mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    
    mpz_init_set(mpzN, n.get_mpz_t());
    mpz_init(mpzE);
    mpz_sub_ui(mpzE, mpzN, 1);
    mpz_init(mpzR);
    mpz_powm(mpzR, bnTwo.get_mpz_t(), mpzE, mpzN);
    if (mpz_cmp_ui(mpzR, 1) == 0)
    {
        mpz_clear(mpzN);
        mpz_clear(mpzE);
        mpz_clear(mpzR);
        return true;
    }
    // Failed Fermat test, calculate fractional length
    mpz_sub(mpzE, mpzN, mpzR);
    mpz_mul_2exp(mpzR, mpzE, nFractionalBits);
    mpz_tdiv_q(mpzE, mpzR, mpzN);
    unsigned int nFractionalLength = mpz_get_ui(mpzE);
    mpz_clear(mpzN);
    mpz_clear(mpzE);
    mpz_clear(mpzR);
    if (nFractionalLength >= (1 << nFractionalBits))
        return error("FermatProbablePrimalityTest() : fractional assert");
    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;
}

// Test probable primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
// fSophieGermain:
//   true:  n = 2p+1, p prime, aka Cunningham Chain of first kind
//   false: n = 2p-1, p prime, aka Cunningham Chain of second kind
// Return values
//   true: n is probable prime
//   false: n is composite; set fractional length in the nLength output

static bool EulerLagrangeLifchitzPrimalityTest(const mpz_class& n, bool fSophieGermain, unsigned int& nLength)
{
	/*//mpz_class a = 2;
	mpz_class e = (n - 1) >> 1;
	mpz_class r;
	BN_mod_exp(&r, &bnTwo, &e, &n, pctx);
	uint32 nMod8U32 = 0;
	if( n.top > 0 )
		nMod8U32 = n.d[0]&7;
	
	// validate the optimization above:
	//mpz_class nMod8 = n % bnConst8;
	//if( mpz_class(nMod8U32) != nMod8 )
	//	__debugbreak();

	bool fPassedTest = false;
	if (fSophieGermain && (nMod8U32 == 7)) // Euler & Lagrange
		fPassedTest = (r == 1);
	else if (fSophieGermain && (nMod8U32 == 3)) // Lifchitz
		fPassedTest = ((r+1) == n);
	else if ((!fSophieGermain) && (nMod8U32 == 5)) // Lifchitz
		fPassedTest = ((r+1) == n);
	else if ((!fSophieGermain) && (nMod8U32 == 1)) // LifChitz
		fPassedTest = (r == 1);
	else
		return error("EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8U32, (fSophieGermain? "first kind" : "second kind"));

	if (fPassedTest)
		return true;
	// Failed test, calculate fractional length
	r = (r * r) % n; // derive Fermat test remainder
	unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
	if (nFractionalLength >= (1 << nFractionalBits))
		return error("EulerLagrangeLifchitzPrimalityTest() : fractional assert");
	nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
	return false;*/
	// Faster GMP version
    mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    
    mpz_init_set(mpzN, n.get_mpz_t());
    mpz_init(mpzE);
    mpz_sub_ui(mpzE, mpzN, 1);
    mpz_tdiv_q_2exp(mpzE, mpzE, 1);
    mpz_init(mpzR);
    mpz_powm(mpzR, bnTwo.get_mpz_t(), mpzE, mpzN);
    unsigned int nMod8 = mpz_tdiv_ui(mpzN, 8);
    bool fPassedTest = false;
    if (fSophieGermain && (nMod8 == 7)) // Euler & Lagrange
        fPassedTest = !mpz_cmp_ui(mpzR, 1);
    else if (fSophieGermain && (nMod8 == 3)) // Lifchitz
    {
        mpz_t mpzRplusOne;
        mpz_init(mpzRplusOne);
        mpz_add_ui(mpzRplusOne, mpzR, 1);
        fPassedTest = !mpz_cmp(mpzRplusOne, mpzN);
        mpz_clear(mpzRplusOne);
    }
    else if ((!fSophieGermain) && (nMod8 == 5)) // Lifchitz
    {
        mpz_t mpzRplusOne;
        mpz_init(mpzRplusOne);
        mpz_add_ui(mpzRplusOne, mpzR, 1);
        fPassedTest = !mpz_cmp(mpzRplusOne, mpzN);
        mpz_clear(mpzRplusOne);
    }
    else if ((!fSophieGermain) && (nMod8 == 1)) // LifChitz
    {
        fPassedTest = !mpz_cmp_ui(mpzR, 1);
    }
    else
    {
        mpz_clear(mpzN);
        mpz_clear(mpzE);
        mpz_clear(mpzR);
        return error("EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8, (fSophieGermain? "first kind" : "second kind"));
    }
    
    if (fPassedTest)
    {
        mpz_clear(mpzN);
        mpz_clear(mpzE);
        mpz_clear(mpzR);
        return true;
    }
    
    // Failed test, calculate fractional length
    mpz_mul(mpzE, mpzR, mpzR);
    mpz_tdiv_r(mpzR, mpzE, mpzN); // derive Fermat test remainder

    mpz_sub(mpzE, mpzN, mpzR);
    mpz_mul_2exp(mpzR, mpzE, nFractionalBits);
    mpz_tdiv_q(mpzE, mpzR, mpzN);
    unsigned int nFractionalLength = mpz_get_ui(mpzE);
    mpz_clear(mpzN);
    mpz_clear(mpzE);
    mpz_clear(mpzR);
    
    if (nFractionalLength >= (1 << nFractionalBits))
        return error("EulerLagrangeLifchitzPrimalityTest() : fractional assert");
    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;
}

// Proof-of-work Target (prime chain target):
//   format - 32 bit, 8 length bits, 24 fractional length bits

unsigned int nTargetInitialLength = 7; // initial chain length target
unsigned int nTargetMinLength = 6;     // minimum chain length target

unsigned int TargetGetLimit()
{
	return (nTargetMinLength << nFractionalBits);
}

unsigned int TargetGetInitial()
{
	return (nTargetInitialLength << nFractionalBits);
}

unsigned int TargetGetLength(unsigned int nBits)
{
	return ((nBits & TARGET_LENGTH_MASK) >> nFractionalBits);
}

bool TargetSetLength(unsigned int nLength, unsigned int& nBits)
{
	if (nLength >= 0xff)
		return error("TargetSetLength() : invalid length=%u", nLength);
	nBits &= TARGET_FRACTIONAL_MASK;
	nBits |= (nLength << nFractionalBits);
	return true;
}

void TargetIncrementLength(unsigned int& nBits)
{
	nBits += (1 << nFractionalBits);
}

void TargetDecrementLength(unsigned int& nBits)
{
	if (TargetGetLength(nBits) > nTargetMinLength)
		nBits -= (1 << nFractionalBits);
}

unsigned int TargetGetFractional(unsigned int nBits)
{
	return (nBits & TARGET_FRACTIONAL_MASK);
}

uint64 TargetGetFractionalDifficulty(unsigned int nBits)
{
	return (nFractionalDifficultyMax / (uint64) ((1ull<<nFractionalBits) - TargetGetFractional(nBits)));
}

bool TargetSetFractionalDifficulty(uint64 nFractionalDifficulty, unsigned int& nBits)
{
	if (nFractionalDifficulty < nFractionalDifficultyMin)
		return error("TargetSetFractionalDifficulty() : difficulty below min");
	uint64 nFractional = nFractionalDifficultyMax / nFractionalDifficulty;
	if (nFractional > (1u<<nFractionalBits))
		return error("TargetSetFractionalDifficulty() : fractional overflow: nFractionalDifficulty=%016I64d", nFractionalDifficulty);
	nFractional = (1u<<nFractionalBits) - nFractional;
	nBits &= TARGET_LENGTH_MASK;
	nBits |= (unsigned int)nFractional;
	return true;
}

std::string TargetToString(unsigned int nBits)
{
	__debugbreak(); // return strprintf("%02x.%06x", TargetGetLength(nBits), TargetGetFractional(nBits));
	return NULL; // todo
}

unsigned int TargetFromInt(unsigned int nLength)
{
	return (nLength << nFractionalBits);
}

// Get mint value from target
// Primecoin mint rate is determined by target
//   mint = 999 / (target length ** 2)
// Inflation is controlled via Moore's Law
bool TargetGetMint(unsigned int nBits, uint64& nMint)
{
	nMint = 0;
	static uint64 nMintLimit = 999ull * COIN;
	CBigNum bnMint = nMintLimit;
	if (TargetGetLength(nBits) < nTargetMinLength)
		return error("TargetGetMint() : length below minimum required, nBits=%08x", nBits);
	bnMint = (bnMint << nFractionalBits) / nBits;
	bnMint = (bnMint << nFractionalBits) / nBits;
	bnMint = (bnMint / CENT) * CENT;  // mint value rounded to cent
	nMint = bnMint.getuint256().Get64();
	if (nMint > nMintLimit)
	{
		nMint = 0;
		return error("TargetGetMint() : mint value over limit, nBits=%08x", nBits);
	}
	return true;
}

// Get next target value
bool TargetGetNext(unsigned int nBits, int64 nInterval, int64 nTargetSpacing, int64 nActualSpacing, unsigned int& nBitsNext)
{
	nBitsNext = nBits;
	// Convert length into fractional difficulty
	uint64 nFractionalDifficulty = TargetGetFractionalDifficulty(nBits);
	// Compute new difficulty via exponential moving toward target spacing
	CBigNum bnFractionalDifficulty = nFractionalDifficulty;
	bnFractionalDifficulty *= ((nInterval + 1) * nTargetSpacing);
	bnFractionalDifficulty /= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
	if (bnFractionalDifficulty > nFractionalDifficultyMax)
		bnFractionalDifficulty = nFractionalDifficultyMax;
	if (bnFractionalDifficulty < nFractionalDifficultyMin)
		bnFractionalDifficulty = nFractionalDifficultyMin;
	uint64 nFractionalDifficultyNew = bnFractionalDifficulty.getuint256().Get64();
	//if (fDebug && GetBoolArg("-printtarget"))
	//	printf("TargetGetNext() : nActualSpacing=%d nFractionDiff=%016"PRI64x" nFractionDiffNew=%016"PRI64x"\n", (int)nActualSpacing, nFractionalDifficulty, nFractionalDifficultyNew);
	// Step up length if fractional past threshold
	if (nFractionalDifficultyNew > nFractionalDifficultyThreshold)
	{
		nFractionalDifficultyNew = nFractionalDifficultyMin;
		TargetIncrementLength(nBitsNext);
	}
	// Step down length if fractional at minimum
	else if (nFractionalDifficultyNew == nFractionalDifficultyMin && TargetGetLength(nBitsNext) > nTargetMinLength)
	{
		nFractionalDifficultyNew = nFractionalDifficultyThreshold;
		TargetDecrementLength(nBitsNext);
	}
	// Convert fractional difficulty back to length
	if (!TargetSetFractionalDifficulty(nFractionalDifficultyNew, nBitsNext))
		return error("TargetGetNext() : unable to set fractional difficulty prev=0x%016I64d new=0x%016I64d", nFractionalDifficulty, nFractionalDifficultyNew);
	return true;
}


// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (length at least 2)
//   false - Not Cunningham Chain
static bool ProbableCunninghamChainTest(const mpz_class& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength)
{
	nProbableChainLength = 0;
	mpz_class N = n;

	// Fermat test for n first
	if (!FermatProbablePrimalityTest(N, nProbableChainLength))
		return false;

	// Euler-Lagrange-Lifchitz test for the following numbers in chain
	while (true)
	{
		TargetIncrementLength(nProbableChainLength);
		N = N + N + (fSophieGermain? 1 : (-1));
		if (fFermatTest)
		{
			if (!FermatProbablePrimalityTest(N, nProbableChainLength))
				break;
		}
		else
		{
			if (!EulerLagrangeLifchitzPrimalityTest(N, fSophieGermain, nProbableChainLength))
				break;
		}
	}

	return (TargetGetLength(nProbableChainLength) >= 2);
}

// Test probable prime chain for: nOrigin
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
bool ProbablePrimeChainTest(const mpz_class& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin)
{
	nChainLengthCunningham1 = 0;
	nChainLengthCunningham2 = 0;
	nChainLengthBiTwin = 0;
	// Test for Cunningham Chain of first kind
	ProbableCunninghamChainTest(bnPrimeChainOrigin-1, true, fFermatTest, nChainLengthCunningham1);
	// Test for Cunningham Chain of second kind
	ProbableCunninghamChainTest(bnPrimeChainOrigin+1, false, fFermatTest, nChainLengthCunningham2);
	// Figure out BiTwin Chain length
	// BiTwin Chain allows a single prime at the end for odd length chain
	nChainLengthBiTwin =
		(TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
		(nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
	(nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));
	return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
}

//// Sieve for mining
//boost::thread_specific_ptr<CSieveOfEratosthenes> psieve;

// Mine probable prime chain of form: n = h * p# +/- 1


bool MineProbablePrimeChain(CSieveOfEratosthenes** psieve, primecoinBlock_t* block, mpz_class& bnFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit)
{
	nProbableChainLength = 0;
	nTests = 0;
	nPrimesHit = 0;
	if (fNewBlock && *psieve != NULL)
	{
		// Must rebuild the sieve
		delete *psieve;
		*psieve = NULL;
	}
	fNewBlock = false;

	//int64 nStart, nCurrent; // microsecond timer
	if (*psieve == NULL)
	{
		// Build sieve
		uint32 nStart = GetTickCount();
		//uint32 sieveStopTimeLimit = nStart + 2000;
		//for(uint32 i=0; i<256/8; i++)
		//	printf("%02x", block->blockHeaderHash.begin()[i]);
		*psieve = new CSieveOfEratosthenes(commandlineInput.sieveSize, block->nBits, block->blockHeaderHash, bnFixedMultiplier);
		if( commandlineInput.sieveForTwinChains )
			(*psieve)->WeaveFastAll();
		else
			(*psieve)->WeaveFastAll_noTwin();
		//printf("MineProbablePrimeChain() : new sieve (%u/%u) ready in %ums multiplier: %u\n", (*psieve)->GetCandidateCount(), commandlineInput.sieveSize, (unsigned int) (GetTickCount() - nStart), bnFixedMultiplier);
	}

	mpz_class bnChainOrigin;

	//nStart = GetTickCount();
	//nCurrent = nStart;

	//uint32 timeStop = GetTickCount() + 25000;
	while ( block->serverData.blockHeight == jhMiner_getCurrentWorkBlockHeight(block->threadIndex) )
	{
		nTests++;
		if (!(*psieve)->GetNextCandidateMultiplier(nTriedMultiplier))
		{
			// power tests completed for the sieve
			delete *psieve;
			*psieve = NULL;
			fNewBlock = true; // notify caller to change nonce
			return false;
		}
		//bnChainOrigin = mpz_class(block->blockHeaderHash) * bnFixedMultiplier * nTriedMultiplier;

		mpz_class mpzPrimeChainMultiplier = bnFixedMultiplier * nTriedMultiplier;
		mpz_class mpzHash;
		mpz_set_uint256(mpzHash.get_mpz_t(), block->blockHeaderHash);
		bnChainOrigin = mpzHash * mpzPrimeChainMultiplier;
		unsigned int nChainLengthCunningham1 = 0;
		unsigned int nChainLengthCunningham2 = 0;
		unsigned int nChainLengthBiTwin = 0;
		
		//add some stuff here


		bool canSubmitAsShare = ProbablePrimeChainTest(bnChainOrigin, block->nBits, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin);
		nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);
		if( nProbableChainLength >= block->serverData.nBitsForShare )
			canSubmitAsShare = true;
		//if( nBitsGen >= 0x03000000 )
		//	printf("%08X\n", nBitsGen);
		if( nProbableChainLength >= 0x01000000 )
			primeStats.numPrimeChainsFound++;
		primeStats.numTestedCandidates++;
		//if( nProbableChainLength > 0x03000000 )
		//	primeStats.qualityPrimesFound++;
		if( nProbableChainLength > primeStats.bestPrimeChainDifficulty )
			primeStats.bestPrimeChainDifficulty = nProbableChainLength;
		if(canSubmitAsShare)
		{
			block->bnPrimeChainMultiplier = bnFixedMultiplier * nTriedMultiplier;
			// update server data
			block->serverData.client_shareBits = nProbableChainLength;
			// generate block raw data
			uint8 blockRawData[256] = {0};
			memcpy(blockRawData, block, 80);
			uint32 writeIndex = 80;
			sint32 lengthBN = 0;
			CBigNum bnPrimeChainMultiplier;
			
			bnPrimeChainMultiplier.SetHex(block->bnPrimeChainMultiplier.get_str(16));


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
			// submit this share
			jhMiner_pushShare_primecoin(blockRawData, block);
			//printf("Probable prime chain found for block=%s!!\n  Target: %s\n  Length: (%s %s %s)\n", block.GetHash().GetHex().c_str(),TargetToString(block.nBits).c_str(), TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
			//nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);
			// since we are using C structs here we have to make sure the memory for the mpz_class in the block is freed again
			//delete *psieve;
			//*psieve = NULL;
			//block->bnPrimeChainMultiplier = NULL;
			RtlZeroMemory(blockRawData, 256);
			//delete *psieve;
			//*psieve = NULL;
			// dont quit if we find a share, there could be other shares in the remaining prime candidates
			//return false;
		}
		//if(TargetGetLength(nProbableChainLength) >= 1)
		//	nPrimesHit++;
		//nCurrent = GetTickCount();
	}
	if( *psieve )
	{
		delete *psieve;
		*psieve = NULL;
	}
	return false; // stop as timed out
}

//// Check prime proof-of-work
//bool CheckPrimeProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const mpz_class& bnPrimeChainMultiplier, unsigned int& nChainType, unsigned int& nChainLength)
//{
//	// Check target
//	if (TargetGetLength(nBits) < nTargetMinLength || TargetGetLength(nBits) > 99)
//		return error("CheckPrimeProofOfWork() : invalid chain length target %s", TargetToString(nBits).c_str());
//
//	// Check header hash limit
//	if (hashBlockHeader < hashBlockHeaderLimit)
//		return error("CheckPrimeProofOfWork() : block header hash under limit");
//	// Check target for prime proof-of-work
//	mpz_class bnPrimeChainOrigin = mpz_class(hashBlockHeader) * bnPrimeChainMultiplier;
//	if (bnPrimeChainOrigin < bnPrimeMin)
//		return error("CheckPrimeProofOfWork() : prime too small");
//	// First prime in chain must not exceed cap
//	if (bnPrimeChainOrigin > bnPrimeMax)
//		return error("CheckPrimeProofOfWork() : prime too big");
//
//	// Check prime chain
//	unsigned int nChainLengthCunningham1 = 0;
//	unsigned int nChainLengthCunningham2 = 0;
//	unsigned int nChainLengthBiTwin = 0;
//	if (!ProbablePrimeChainTest(bnPrimeChainOrigin, nBits, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin))
//		return error("CheckPrimeProofOfWork() : failed prime chain test target=%s length=(%s %s %s)", TargetToString(nBits).c_str(),
//		TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
//	if (nChainLengthCunningham1 < nBits && nChainLengthCunningham2 < nBits && nChainLengthBiTwin < nBits)
//		return error("CheckPrimeProofOfWork() : prime chain length assert target=%s length=(%s %s %s)", TargetToString(nBits).c_str(),
//		TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str());
//
//	// Double check prime chain with Fermat tests only
//	unsigned int nChainLengthCunningham1FermatTest = 0;
//	unsigned int nChainLengthCunningham2FermatTest = 0;
//	unsigned int nChainLengthBiTwinFermatTest = 0;
//	if (!ProbablePrimeChainTest(bnPrimeChainOrigin, nBits, true, nChainLengthCunningham1FermatTest, nChainLengthCunningham2FermatTest, nChainLengthBiTwinFermatTest))
//		return error("CheckPrimeProofOfWork() : failed Fermat test target=%s length=(%s %s %s) lengthFermat=(%s %s %s)", TargetToString(nBits).c_str(),
//		TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str(),
//		TargetToString(nChainLengthCunningham1FermatTest).c_str(), TargetToString(nChainLengthCunningham2FermatTest).c_str(), TargetToString(nChainLengthBiTwinFermatTest).c_str());
//	if (nChainLengthCunningham1 != nChainLengthCunningham1FermatTest ||
//		nChainLengthCunningham2 != nChainLengthCunningham2FermatTest ||
//		nChainLengthBiTwin != nChainLengthBiTwinFermatTest)
//		return error("CheckPrimeProofOfWork() : failed Fermat-only double check target=%s length=(%s %s %s) lengthFermat=(%s %s %s)", TargetToString(nBits).c_str(), 
//		TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str(),
//		TargetToString(nChainLengthCunningham1FermatTest).c_str(), TargetToString(nChainLengthCunningham2FermatTest).c_str(), TargetToString(nChainLengthBiTwinFermatTest).c_str());
//
//	// Select the longest primechain from the three chain types
//	nChainLength = nChainLengthCunningham1;
//	nChainType = PRIME_CHAIN_CUNNINGHAM1;
//	if (nChainLengthCunningham2 > nChainLength)
//	{
//		nChainLength = nChainLengthCunningham2;
//		nChainType = PRIME_CHAIN_CUNNINGHAM2;
//	}
//	if (nChainLengthBiTwin > nChainLength)
//	{
//		nChainLength = nChainLengthBiTwin;
//		nChainType = PRIME_CHAIN_BI_TWIN;
//	}
//
//	// Check that the certificate (bnPrimeChainMultiplier) is normalized
//	if (bnPrimeChainMultiplier % 2 == 0 && bnPrimeChainOrigin % 4 == 0)
//	{
//		unsigned int nChainLengthCunningham1Extended = 0;
//		unsigned int nChainLengthCunningham2Extended = 0;
//		unsigned int nChainLengthBiTwinExtended = 0;
//		if (ProbablePrimeChainTest(bnPrimeChainOrigin / 2, nBits, false, nChainLengthCunningham1Extended, nChainLengthCunningham2Extended, nChainLengthBiTwinExtended))
//		{ // try extending down the primechain with a halved multiplier
//			if (nChainLengthCunningham1Extended > nChainLength || nChainLengthCunningham2Extended > nChainLength || nChainLengthBiTwinExtended > nChainLength)
//				return error("CheckPrimeProofOfWork() : prime certificate not normalzied target=%s length=(%s %s %s) extend=(%s %s %s)",
//				TargetToString(nBits).c_str(),
//				TargetToString(nChainLengthCunningham1).c_str(), TargetToString(nChainLengthCunningham2).c_str(), TargetToString(nChainLengthBiTwin).c_str(),
//				TargetToString(nChainLengthCunningham1Extended).c_str(), TargetToString(nChainLengthCunningham2Extended).c_str(), TargetToString(nChainLengthBiTwinExtended).c_str());
//		}
//	}
//
//	return true;
//}

// prime target difficulty value for visualization
double GetPrimeDifficulty(unsigned int nBits)
{
	return ((double) nBits / (double) (1 << nFractionalBits));
}

// Estimate work transition target to longer prime chain
unsigned int EstimateWorkTransition(unsigned int nPrevWorkTransition, unsigned int nBits, unsigned int nChainLength)
{
	int64 nInterval = 500;
	int64 nWorkTransition = nPrevWorkTransition;
	unsigned int nBitsCeiling = 0;
	TargetSetLength(TargetGetLength(nBits)+1, nBitsCeiling);
	unsigned int nBitsFloor = 0;
	TargetSetLength(TargetGetLength(nBits), nBitsFloor);
	uint64 nFractionalDifficulty = TargetGetFractionalDifficulty(nBits);
	bool fLonger = (TargetGetLength(nChainLength) > TargetGetLength(nBits));
	if (fLonger)
		nWorkTransition = (nWorkTransition * (((nInterval - 1) * nFractionalDifficulty) >> 32) + 2 * ((uint64) nBitsFloor)) / ((((nInterval - 1) * nFractionalDifficulty) >> 32) + 2);
	else
		nWorkTransition = ((nInterval - 1) * nWorkTransition + 2 * ((uint64) nBitsCeiling)) / (nInterval + 1);
	return nWorkTransition;
}

/* akruppa's code */
int single_modinv (int a, int modulus)
{ /* start of single_modinv */
	int ps1, ps2, parity, dividend, divisor, rem, q, t;
	q = 1;
	rem = a;
	dividend = modulus;
	divisor = a;
	ps1 = 1;
	ps2 = 0;
	parity = 0;
	while (divisor > 1)
	{
		rem = dividend - divisor;
		t = rem - divisor;
		if (t >= 0) {
			q += ps1;
			rem = t;
			t -= divisor;
			if (t >= 0) {
				q += ps1;
				rem = t;
				t -= divisor;
				if (t >= 0) {
					q += ps1;
					rem = t;
					t -= divisor;
					if (t >= 0) {
						q += ps1;
						rem = t;
						t -= divisor;
						if (t >= 0) {
							q += ps1;
							rem = t;
							t -= divisor;
							if (t >= 0) {
								q += ps1;
								rem = t;
								t -= divisor;
								if (t >= 0) {
									q += ps1;
									rem = t;
									t -= divisor;
									if (t >= 0) {
										q += ps1;
										rem = t;
										if (rem >= divisor) {
											q = dividend/divisor;
											rem = dividend - q * divisor;
											q *= ps1;
										}}}}}}}}}
		q += ps2;
		parity = ~parity;
		dividend = divisor;
		divisor = rem;
		ps2 = ps1;
		ps1 = q;
	}

	if (parity == 0)
		return (ps1);
	else
		return (modulus - ps1);
} /* end of single_modinv */


// Weave sieve for the next prime in table
// Return values:
//   True  - weaved another prime; nComposite - number of composites removed
//   False - sieve already completed
bool CSieveOfEratosthenes::WeaveFastAll()
{
	//mpz_class p = vPrimes[nPrimeSeq];
	// init some required bignums on the stack (no dynamic allocation at all)
	//BIGNUM bn_p;
	//BIGNUM bn_tmp;
	BIGNUM bn_fixedInverse;
	BIGNUM bn_twoInverse;
	BIGNUM bn_fixedMod;
	//uint32 bignumData_p[0x200/4];
	//uint32 bignumData_tmp[0x200/4];
	uint32 bignumData_fixedInverse[0x200/4];
	uint32 bignumData_twoInverse[0x200/4];
	uint32 bignumData_fixedMod[0x200/4];
	//fastInitBignum(bn_p, bignumData_p);
	//fastInitBignum(bn_tmp, bignumData_tmp);
	fastInitBignum(bn_fixedInverse, bignumData_fixedInverse);
	fastInitBignum(bn_twoInverse, bignumData_twoInverse);
	fastInitBignum(bn_fixedMod, bignumData_fixedMod);

	unsigned int nChainLength = TargetGetLength(nBits);
	unsigned int nChainLengthX2 = nChainLength*2;

	for(uint32 nPrimeSeq=0; nPrimeSeq<vPrimesSize; nPrimeSeq++)
	{
		uint32 fixedFactorMod = BN2_mod_word(&bnFixedFactor, vPrimes[nPrimeSeq]);
		//if (bnFixedFactor % p == 0)
		if( fixedFactorMod == 0 )
		{
			// Nothing in the sieve is divisible by this prime
			continue;
		}

		//mpz_class p = mpz_class(vPrimes[nPrimeSeq]);
		// debug: Code is correct until here

		// Find the modulo inverse of fixed factor
		//if (!BN2_mod_inverse(&bn_fixedInverse, &bnFixedFactor, &bn_p, pctx))
		//	return error("CSieveOfEratosthenes::Weave(): BN_mod_inverse of fixed factor failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
		//

		//sint32 djf1 = single_modinv_ak(bn_tmp.d[0], vPrimes[nPrimeSeq]);
		//sint32 djf2 = int_invert(bn_tmp.d[0], vPrimes[nPrimeSeq]);
		//if( djf1 != djf2 )
		//	__debugbreak();

		//BN_set_word(&bn_fixedInverse, );

		//mpz_class bnTwo = 2;
		//if (!BN_mod_inverse(&bn_twoInverse, &bn_constTwo, &bn_p, pctx))
		//	return error("CSieveOfEratosthenes::Weave(): BN_mod_inverse of 2 failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
		//if( BN_cmp(&bn_twoInverse, &mpz_class(vPrimesTwoInverse[nPrimeSeq])) )
		//	__debugbreak();

		////BN_set_word(&bn_twoInverse, vPrimesTwoInverse[nPrimeSeq]);

		uint64 pU64 = (uint64)vPrimes[nPrimeSeq];
		uint64 fixedInverseU64 = single_modinv(fixedFactorMod, vPrimes[nPrimeSeq]);//BN_get_word(&bn_fixedInverse);
		uint64 twoInverseU64 = vPrimesTwoInverse[nPrimeSeq];//BN_get_word(&bn_twoInverse);
		// Weave the sieve for the prime
		for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < nChainLengthX2; nBiTwinSeq++)
		{
			// Find the first number that's divisible by this prime
			//BN_copy(&bn_tmp, &bn_p);
			//if( (nBiTwinSeq&1) == 0 )
			//	BN_add_word(&bn_tmp, 1);
			//else
			//	BN_sub_word(&bn_tmp, 1);		
			//BN_mul(&bn_tmp, &bn_fixedInverse, &bn_tmp, pctx);
			//BN_mod(&bn_tmp, &bn_tmp, &bn_p, pctx);
			//unsigned int nSolvedMultiplier = BN_get_word(&bn_tmp);

			uint64 nSolvedMultiplier;
			if( (nBiTwinSeq&1) == 0 )
				nSolvedMultiplier = ((fixedInverseU64) * (pU64 + 1ULL)) % pU64;
			else
				nSolvedMultiplier = ((fixedInverseU64) * (pU64 - 1ULL)) % pU64;

			//if( nSolvedMultiplier != nSolvedMultiplier2 )
			//	__debugbreak();



			if (nBiTwinSeq % 2 == 1)
			{
				fixedInverseU64 = (fixedInverseU64*twoInverseU64)%pU64;
			}

			unsigned int nPrime = vPrimes[nPrimeSeq];
			if (nBiTwinSeq < nChainLength)
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeBiTwin[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);
			if ( nBiTwinSeq & 1 )
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeCunningham2[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);
			else
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeCunningham1[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);
		}
	}
	return false; // never reached
}

// Weave sieve for the next prime in table, ignores twin chains (only searches for Cunningham Chains)
// Return values:
//   True  - weaved another prime; nComposite - number of composites removed
//   False - sieve already completed
bool CSieveOfEratosthenes::WeaveFastAll_noTwin()
{
	//mpz_class p = vPrimes[nPrimeSeq];
	// init some required bignums on the stack (no dynamic allocation at all)
	//BIGNUM bn_p;
	//BIGNUM bn_tmp;
	BIGNUM bn_fixedInverse;
	BIGNUM bn_twoInverse;
	BIGNUM bn_fixedMod;
	//uint32 bignumData_p[0x200/4];
	//uint32 bignumData_tmp[0x200/4];
	uint32 bignumData_fixedInverse[0x200/4];
	uint32 bignumData_twoInverse[0x200/4];
	uint32 bignumData_fixedMod[0x200/4];
	//fastInitBignum(bn_p, bignumData_p);
	//fastInitBignum(bn_tmp, bignumData_tmp);
	fastInitBignum(bn_fixedInverse, bignumData_fixedInverse);
	fastInitBignum(bn_twoInverse, bignumData_twoInverse);
	fastInitBignum(bn_fixedMod, bignumData_fixedMod);

	unsigned int nChainLength = TargetGetLength(nBits);
	for(uint32 nPrimeSeq=0; nPrimeSeq<vPrimesSize; nPrimeSeq++)
	{
		uint32 fixedFactorMod = BN2_mod_word(&bnFixedFactor, vPrimes[nPrimeSeq]);
		if( fixedFactorMod == 0 )
		{
			// Nothing in the sieve is divisible by this prime
			continue;
		}

		uint64 pU64 = (uint64)vPrimes[nPrimeSeq];
		uint64 fixedInverseU64 = single_modinv(fixedFactorMod, vPrimes[nPrimeSeq]);//BN_get_word(&bn_fixedInverse);
		uint64 twoInverseU64 = vPrimesTwoInverse[nPrimeSeq];//BN_get_word(&bn_twoInverse);
		// Weave the sieve for the prime
		uint64 nSolvedMultiplier;
		unsigned int nPrime = vPrimes[nPrimeSeq];
		for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < nChainLength; nBiTwinSeq++)
		{
			// first pass
			nSolvedMultiplier = ((fixedInverseU64) * (pU64 + 1ULL)) % pU64;
			for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
				vfCompositeCunningham1[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);
			// second pass
			nSolvedMultiplier = ((fixedInverseU64) * (pU64 - 1ULL)) % pU64;
			fixedInverseU64 = (fixedInverseU64*twoInverseU64)%pU64;
			for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
				vfCompositeCunningham2[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);
		}
	}
	return false; // never reached
}