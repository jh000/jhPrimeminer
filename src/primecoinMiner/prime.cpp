// Copyright (c) 2013 Primecoin developers
// Distributed under conditional MIT/X11 software license,
// see the accompanying file COPYING

#include "global.h"
#include <bitset>
#include <time.h>


// Prime Table
//std::vector<unsigned int> vPrimes;
//uint32* vPrimes;
//uint32* vPrimesTwoInverse;
//uint32 vPrimesSize = 0;

__declspec( thread ) BN_CTX* pctx = NULL;

// changed to return the ticks since reboot
// doesnt need to be the correct time, just a more or less random input value
uint64 GetTimeMicros()
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (uint64)t.QuadPart;
}

uint32* vPrimes;
uint32	vPrimesSize;
uint32* vTwoInverses;
uint32	vTwoInversesSize;

unsigned int int_invert(unsigned int a, unsigned int nPrime);

void GeneratePrimeTable(unsigned int nSieveSize)
{
    const unsigned int nPrimeTableLimit = 20000000;
	vPrimes = (uint32*)malloc(sizeof(uint32)*(nPrimeTableLimit/4));
	// Generate prime table using sieve of Eratosthenes
    //std::vector<bool> vfComposite (nPrimeTableLimit, false);
	uint8* vfComposite = (uint8*)malloc(sizeof(uint8)*(nPrimeTableLimit+7)/8);
	memset(vfComposite, 0x00, sizeof(uint8)*(nPrimeTableLimit+7)/8);
	for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
	{
		//if (vfComposite[nFactor])
		if( vfComposite[nFactor>>3] & (1<<(nFactor&7)) )
			continue;
		for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
			vfComposite[nComposite>>3] |= 1<<(nComposite&7);
	}
	for (unsigned int n = 2; n < nPrimeTableLimit; n++)
	{
		if ( (vfComposite[n>>3] & (1<<(n&7)))==0 )
		{
			vPrimes[vPrimesSize] = n;
			vPrimesSize++;
		}
	}
	vPrimes = (uint32*)realloc(vPrimes, sizeof(uint32)*vPrimesSize);
    printf("GeneratePrimeTable() : prime table [1, %d] generated with %lu primes\n", nPrimeTableLimit, vPrimesSize);
    vTwoInverses = (uint32*)malloc(sizeof(uint32)*vPrimesSize);
    for (unsigned int nPrimeSeq = 1; nPrimeSeq < vPrimesSize; nPrimeSeq++)
	{
        vTwoInverses[nPrimeSeq] = int_invert(2, vPrimes[nPrimeSeq]);
	}
	free(vfComposite);
}

// Get next prime number of p
bool PrimeTableGetNextPrime(unsigned int& p)
{
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];

        if ( nPrime > p)
		{
            p = nPrime;
			return true;
		}
	}
	return false;
}

// Get previous prime number of p
bool PrimeTableGetPreviousPrime(unsigned int& p)
{
	uint32 nPrevPrime = 0;
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		if (vPrimes[i] >= p)
			break;
		nPrevPrime = vPrimes[i];
	}
	if (nPrevPrime)
	{
		p = nPrevPrime;
		return true;
	}
	return false;
}

// Compute Primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial)
{
	bnPrimorial = 1;
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];
		if (nPrime > p) break;
		bnPrimorial *= nPrime;
	}
}


// Compute Primorial number p#
void Primorial(unsigned int p, mpz_class& mpzPrimorial)
{
	mpzPrimorial = 1;
	//BOOST_FOREACH(unsigned int nPrime, vPrimes)
	//for(uint32 i=0; i<vPrimes.size(); i++)
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];
		if (nPrime > p) break;
		mpzPrimorial *= nPrime;
	}
}

// Compute Primorial number p#
// Fast 32-bit version assuming that p <= 23
unsigned int PrimorialFast(unsigned int p)
{
    unsigned int nPrimorial = 1;
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];
		if (nPrime > p) break;
        nPrimorial *= nPrime;
	}
    return nPrimorial;
}

// Compute first primorial number greater than or equal to pn
void PrimorialAt(mpz_class& bn, mpz_class& mpzPrimorial)
{
	mpzPrimorial = 1;
	//BOOST_FOREACH(unsigned int nPrime, vPrimes)
	//for(uint32 i=0; i<vPrimes.size(); i++)
	for(uint32 i=0; i<vPrimesSize; i++)
	{
		unsigned int nPrime = vPrimes[i];
		mpzPrimorial *= nPrime;
		if (mpzPrimorial >= bn)
			return;
	}
}


// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTest(const CBigNum& n, unsigned int& nLength)
{
	//CBigNum a = 2; // base; Fermat witness
	CBigNum e = n - 1;
	CBigNum r;
	BN_mod_exp(&r, &bnTwo, &e, &n, pctx);
	if (r == 1)
		return true;
	// Failed Fermat test, calculate fractional length
	unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
	if (nFractionalLength >= (1 << nFractionalBits))
		return error("FermatProbablePrimalityTest() : fractional assert");
	nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
	return false;
}


// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTest(const mpz_class& n, unsigned int& nLength)
{
    // Faster GMP version
    
    mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    
    mpz_init_set(mpzN, n.get_mpz_t());
    mpz_init(mpzE);
    mpz_sub_ui(mpzE, mpzN, 1);
    mpz_init(mpzR);
    mpz_powm(mpzR, mpzTwo.get_mpz_t(), mpzE, mpzN);
    if (mpz_cmp_ui(mpzR, 1) == 0) {
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

static bool EulerLagrangeLifchitzPrimalityTest(const CBigNum& n, bool fSophieGermain, unsigned int& nLength)
{
	//CBigNum a = 2;
	CBigNum e = (n - 1) >> 1;
	CBigNum r;
	BN_mod_exp(&r, &bnTwo, &e, &n, pctx);
	uint32 nMod8U32 = 0;
	if( n.top > 0 )
		nMod8U32 = n.d[0]&7;
	
	// validate the optimization above:
	//CBigNum nMod8 = n % bnConst8;
	//if( CBigNum(nMod8U32) != nMod8 )
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
	return false;
}

void
mpz_powm2 (mpz_ptr r, mpz_srcptr b, mpz_srcptr e, mpz_srcptr m);

// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTestFast(const mpz_class& n, unsigned int& nLength, CPrimalityTestParams& testParams, bool doTrialDivision = false, bool fFastFail = false)
{
    // Faster GMP version
    mpz_t& mpzE = testParams.mpzE;
    mpz_t& mpzR = testParams.mpzR;

    //if(doTrialDivision)
    //{
    //    // Fast divisibility tests
    //    // Divide n by a large divisor
    //    // Use the remainder to test divisibility by small primes
    //    const unsigned int nDivSize = testParams.nFastDivisorsSize;
    //    for (unsigned int i = 0; i < nDivSize; i++)
    //    {
    //        unsigned long lRemainder = mpz_tdiv_ui(n.get_mpz_t(), testParams.vFastDivisors[i]);
    //        unsigned int nPrimeSeq = testParams.vFastDivSeq[i];
    //        const unsigned int nPrimeSeqEnd = testParams.vFastDivSeq[i + 1];
    //        for (; nPrimeSeq < nPrimeSeqEnd; nPrimeSeq++)
    //        {
    //            if (lRemainder % vPrimes[nPrimeSeq] == 0)
				//{
				//	uint32 modRes = mpz_tdiv_ui(n.get_mpz_t(), vPrimes[nPrimeSeq]);
				//	mpz_class mpzTest;
				//	uint32 modRes2 = mpz_mod_ui(mpzTest.get_mpz_t(), n.get_mpz_t(), vPrimes[nPrimeSeq]);

    //                return false;
				//}
    //        }
    //    }
    //}

    mpz_sub_ui(mpzE, n.get_mpz_t(), 1);
	mpz_powm2(mpzR, mpzTwo.get_mpz_t(), mpzE, n.get_mpz_t());
	
	// validate result
	//mpz_t mpzR2 = {0};
	//mpz_powm(mpzR2, mpzTwo.get_mpz_t(), mpzE, n.get_mpz_t());
	////if( mpz_cmp(mpzR, mpzR2) )
	////	__debugbreak();
	//bool mustBeTrue = (mpz_cmp_ui(mpzR2, 1) == 0);
	//if( ((mpz_cmp_ui(mpzR, 1) == 0) == false) && mustBeTrue == true )
	//	__debugbreak();
    if (mpz_cmp_ui(mpzR, 1) == 0)
        return true;
    if (fFastFail)
        return false;
    // Failed Fermat test, calculate fractional length
    mpz_sub(mpzE, n.get_mpz_t(), mpzR);
    mpz_mul_2exp(mpzR, mpzE, nFractionalBits);
    mpz_tdiv_q(mpzE, mpzR, n.get_mpz_t());
    unsigned int nFractionalLength = mpz_get_ui(mpzE);
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
static bool EulerLagrangeLifchitzPrimalityTestFast(const mpz_class& n, bool fSophieGermain, unsigned int& nLength, CPrimalityTestParams& testParams/*, bool fFastDiv = false*/)
{
    // Faster GMP version
    mpz_t& mpzE = testParams.mpzE;
    mpz_t& mpzR = testParams.mpzR;
    mpz_t& mpzRplusOne = testParams.mpzRplusOne;
	/*
    if (fFastDiv)
    {
        // Fast divisibility tests
        // Divide n by a large divisor
        // Use the remainder to test divisibility by small primes
        const unsigned int nDivSize = testParams.nFastDivisorsSize;
        for (unsigned int i = 0; i < nDivSize; i++)
        {
            unsigned long lRemainder = mpz_tdiv_ui(n.get_mpz_t(), testParams.vFastDivisors[i]);
            unsigned int nPrimeSeq = testParams.vFastDivSeq[i];
            const unsigned int nPrimeSeqEnd = testParams.vFastDivSeq[i + 1];
            for (; nPrimeSeq < nPrimeSeqEnd; nPrimeSeq++)
            {
                if (lRemainder % vPrimes[nPrimeSeq] == 0)
                    return false;
            }
        }
    }
	*/
    mpz_sub_ui(mpzE, n.get_mpz_t(), 1);
    mpz_tdiv_q_2exp(mpzE, mpzE, 1);
    mpz_powm(mpzR, mpzTwo.get_mpz_t(), mpzE, n.get_mpz_t());
    unsigned int nMod8 = mpz_get_ui(n.get_mpz_t()) % 8;
    bool fPassedTest = false;
    if (fSophieGermain && (nMod8 == 7)) // Euler & Lagrange
        fPassedTest = !mpz_cmp_ui(mpzR, 1);
    else if (fSophieGermain && (nMod8 == 3)) // Lifchitz
    {
        mpz_add_ui(mpzRplusOne, mpzR, 1);
        fPassedTest = !mpz_cmp(mpzRplusOne, n.get_mpz_t());
    }
    else if ((!fSophieGermain) && (nMod8 == 5)) // Lifchitz
    {
        mpz_add_ui(mpzRplusOne, mpzR, 1);
        fPassedTest = !mpz_cmp(mpzRplusOne, n.get_mpz_t());
    }
    else if ((!fSophieGermain) && (nMod8 == 1)) // LifChitz
        fPassedTest = !mpz_cmp_ui(mpzR, 1);
    else
        return error("EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8, (fSophieGermain? "first kind" : "second kind"));
    
    if (fPassedTest)
    {
        return true;
    }
    
    // Failed test, calculate fractional length
    mpz_mul(mpzE, mpzR, mpzR);
    mpz_tdiv_r(mpzR, mpzE, n.get_mpz_t()); // derive Fermat test remainder

    mpz_sub(mpzE, n.get_mpz_t(), mpzR);
    mpz_mul_2exp(mpzR, mpzE, nFractionalBits);
    mpz_tdiv_q(mpzE, mpzR, n.get_mpz_t());
    unsigned int nFractionalLength = mpz_get_ui(mpzE);
    
    if (nFractionalLength >= (1 << nFractionalBits))
        return error("EulerLagrangeLifchitzPrimalityTest() : fractional assert");
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
    // Faster GMP version
    mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    
    mpz_init_set(mpzN, n.get_mpz_t());
    mpz_init(mpzE);
    mpz_sub_ui(mpzE, mpzN, 1);
    mpz_tdiv_q_2exp(mpzE, mpzE, 1);
    mpz_init(mpzR);
    mpz_powm(mpzR, mpzTwo.get_mpz_t(), mpzE, mpzN);
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
    
    if (fPassedTest) {
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
    
    if (nFractionalLength >= (1 << nFractionalBits)) {
        return error("EulerLagrangeLifchitzPrimalityTest() : fractional assert");
    }
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
bool ProbableCunninghamChainTestFast(const mpz_class& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength, CPrimalityTestParams& testParams, bool doTrialDivision)
{
    nProbableChainLength = 0;

    // Fermat test for n first
    if(!FermatProbablePrimalityTestFast(n, nProbableChainLength, testParams, doTrialDivision, true))
        return false;

    // Euler-Lagrange-Lifchitz test for the following numbers in chain
    mpz_class &N = testParams.N;
    N = n;
    while (true)
    {
        TargetIncrementLength(nProbableChainLength);
		N <<= 1;
        N += (fSophieGermain? 1 : (-1));
        if (fFermatTest)
        {
            if (!FermatProbablePrimalityTestFast(N, nProbableChainLength, testParams))
                break;
        }
        else
        {
            if (!EulerLagrangeLifchitzPrimalityTestFast(N, fSophieGermain, nProbableChainLength, testParams))
                break;
        }
    }

    return (TargetGetLength(nProbableChainLength) >= 2);
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

static bool ProbableCunninghamChainTestBN(const CBigNum& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength)
{
	nProbableChainLength = 0;
	CBigNum N = n;

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
bool ProbablePrimeChainTest(const mpz_class& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin, bool fullTest)
{
	nChainLengthCunningham1 = 0;
	nChainLengthCunningham2 = 0;
	nChainLengthBiTwin = 0;

	// Test for Cunningham Chain of second kind
	ProbableCunninghamChainTest(bnPrimeChainOrigin+1, false, fFermatTest, nChainLengthCunningham2);
	if (nChainLengthCunningham2 >= nBits && !fullTest)
		return true;
	// Test for Cunningham Chain of first kind
	ProbableCunninghamChainTest(bnPrimeChainOrigin-1, true, fFermatTest, nChainLengthCunningham1);
	if (nChainLengthCunningham1 >= nBits && !fullTest)
		return true;
	// Figure out BiTwin Chain length
	// BiTwin Chain allows a single prime at the end for odd length chain
	nChainLengthBiTwin =
		(TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
		(nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
	(nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));
	if (fullTest)
		return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
	else
		return nChainLengthBiTwin >= nBits;
}


// Test probable prime chain for: nOrigin
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
bool ProbablePrimeChainTestFast(const mpz_class& mpzPrimeChainOrigin, CPrimalityTestParams& testParams, uint32 sieveFlags)
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

	bool testCFirstKind = ((sieveFlags&SIEVE_FLAG_BT_COMPOSITE)==0) || ((sieveFlags&SIEVE_FLAG_C1_COMPOSITE)==0);
	bool testCSecondKind = ((sieveFlags&SIEVE_FLAG_BT_COMPOSITE)==0) || ((sieveFlags&SIEVE_FLAG_C2_COMPOSITE)==0);

	bool testCFirstFast = ((sieveFlags&SIEVE_FLAG_C1_COMPOSITE)!=0);
	bool testCSecondFast  ((sieveFlags&SIEVE_FLAG_C2_COMPOSITE)!=0);

    // Test for Cunningham Chain of first kind
    mpzOriginMinusOne = mpzPrimeChainOrigin - 1;
	if( testCFirstKind )
	    ProbableCunninghamChainTestFast(mpzOriginMinusOne, true, fFermatTest, nChainLengthCunningham1, testParams, testCFirstFast);
	else
		nChainLengthCunningham1 = 0;
    // Test for Cunningham Chain of second kind
    mpzOriginPlusOne = mpzPrimeChainOrigin + 1;
	if( testCSecondKind )
	    ProbableCunninghamChainTestFast(mpzOriginPlusOne, false, fFermatTest, nChainLengthCunningham2, testParams, testCSecondFast);
	else
		nChainLengthCunningham2 = 0;
	// verify if there is any chance to find a biTwin that worth calculation
	if (nChainLengthCunningham1 < 0x4000000 || nChainLengthCunningham2 < 0x4000000)
		return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits);

    // Figure out BiTwin Chain length
    // BiTwin Chain allows a single prime at the end for odd length chain
    nChainLengthBiTwin =
        (TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
            (nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
            (nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));

    return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
}


bool ProbablePrimeChainTestOrig(const mpz_class& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin, bool fullTest)
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
	if (nChainLengthCunningham1 >= nBits && nChainLengthCunningham1 >= nChainLengthCunningham2 && nChainLengthCunningham1 >= nChainLengthBiTwin)
		primeStats.cunningham1Count++;
	if (nChainLengthCunningham2 >= nBits && nChainLengthCunningham2 >= nChainLengthCunningham1 && nChainLengthCunningham2 >= nChainLengthBiTwin)
		primeStats.cunningham2Count++;
	if (nChainLengthBiTwin >= nBits && nChainLengthBiTwin >= nChainLengthCunningham1 && nChainLengthBiTwin >= nChainLengthCunningham2)
		primeStats.cunninghamBiTwinCount++;
	
    return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);}



bool ProbablePrimeChainTestBN(const CBigNum& bnPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin)
{
	nChainLengthCunningham1 = 0;
	nChainLengthCunningham2 = 0;
	nChainLengthBiTwin = 0;

	// Test for Cunningham Chain of first kind
	ProbableCunninghamChainTestBN(bnPrimeChainOrigin-1, true, fFermatTest, nChainLengthCunningham1);
	if (nChainLengthCunningham1 >= nBits)
		return true;
	// Test for Cunningham Chain of second kind
	ProbableCunninghamChainTestBN(bnPrimeChainOrigin+1, false, fFermatTest, nChainLengthCunningham2);
	// Figure out BiTwin Chain length
	// BiTwin Chain allows a single prime at the end for odd length chain
	nChainLengthBiTwin =
		(TargetGetLength(nChainLengthCunningham1) > TargetGetLength(nChainLengthCunningham2))?
		(nChainLengthCunningham2 + TargetFromInt(TargetGetLength(nChainLengthCunningham2)+1)) :
	(nChainLengthCunningham1 + TargetFromInt(TargetGetLength(nChainLengthCunningham1)));
	return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
}

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

//// prime chain type and length value
//std::string GetPrimeChainName(unsigned int nChainType, unsigned int nChainLength)
//{
//	return strprintf("%s%s", (nChainType==PRIME_CHAIN_CUNNINGHAM1)? "1CC" : ((nChainType==PRIME_CHAIN_CUNNINGHAM2)? "2CC" : "TWN"), TargetToString(nChainLength).c_str());
//}

/*
// Weave sieve for the next prime in table
// Return values:
//   True  - weaved another prime; nComposite - number of composites removed
//   False - sieve already completed
bool CSieveOfEratosthenes::WeaveOriginal()
{
	if (nPrimeSeq >= vPrimesSize || vPrimes[nPrimeSeq] >= nSieveSize)
		return false;  // sieve has been completed
	CBigNum p = vPrimes[nPrimeSeq];
	if (bnFixedFactor % p == 0)
	{
		// Nothing in the sieve is divisible by this prime
		nPrimeSeq++;
		return true;
	}
	// Find the modulo inverse of fixed factor
	CBigNum bnFixedInverse;
	if (!BN_mod_inverse(&bnFixedInverse, &bnFixedFactor, &p, pctx))
		return error("CSieveOfEratosthenes::Weave(): BN_mod_inverse of fixed factor failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
	CBigNum bnTwo = 2;
	CBigNum bnTwoInverse;
	if (!BN_mod_inverse(&bnTwoInverse, &bnTwo, &p, pctx))
		return error("CSieveOfEratosthenes::Weave(): BN_mod_inverse of 2 failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);

	// Weave the sieve for the prime
	unsigned int nChainLength = TargetGetLength(nBits);
	for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < 2 * nChainLength; nBiTwinSeq++)
	{
		// Find the first number that's divisible by this prime
		int nDelta = ((nBiTwinSeq % 2 == 0)? (-1) : 1);
		unsigned int nSolvedMultiplier = ((bnFixedInverse * (p - nDelta)) % p).getuint();
		if (nBiTwinSeq % 2 == 1)
			bnFixedInverse *= bnTwoInverse; // for next number in chain

		unsigned int nPrime = vPrimes[nPrimeSeq];
		if (nBiTwinSeq < nChainLength)
			for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
				vfCompositeBiTwin[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
		if (((nBiTwinSeq & 1u) == 0))
			for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
				vfCompositeCunningham1[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
		if (((nBiTwinSeq & 1u) == 1u))
			for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
				vfCompositeCunningham2[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
	}
	nPrimeSeq++;
	//delete[] p;
	return true;
}
*/



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

unsigned int int_invert(unsigned int a, unsigned int nPrime)
{
    // Extended Euclidean algorithm to calculate the inverse of a in finite field defined by nPrime
    int rem0 = nPrime, rem1 = a % nPrime, rem2;
    int aux0 = 0, aux1 = 1, aux2;
    int quotient, inverse;

    while (1)
    {
        if (rem1 <= 1)
        {
            inverse = aux1;
            break;
        }

        rem2 = rem0 % rem1;
        quotient = rem0 / rem1;
        aux2 = -quotient * aux1 + aux0;

        if (rem2 <= 1)
        {
            inverse = aux2;
            break;
        }

        rem0 = rem1 % rem2;
        quotient = rem1 / rem2;
        aux0 = -quotient * aux2 + aux1;

        if (rem0 <= 1)
        {
            inverse = aux0;
            break;
        }

        rem1 = rem2 % rem0;
        quotient = rem2 / rem0;
        aux1 = -quotient * aux0 + aux2;
    }

    return (inverse + nPrime) % nPrime;
}