// Copyright (c) 2013 Primecoin developers
// Distributed under conditional MIT/X11 software license,
// see the accompanying file COPYING

#include "global.h"
#include <bitset>
#include <time.h>


// Prime Table
//std::vector<unsigned int> vPrimes;
//uint32* vPrimes;
uint32* vPrimesTwoInverse;
uint32 vPrimesSize = 0;

__declspec( thread ) BN_CTX* pctx = NULL;

// changed to return the ticks since reboot
// doesnt need to be the correct time, just a more or less random input value
uint64 GetTimeMicros()
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (uint64)t.QuadPart;
}


std::vector<unsigned int> vPrimes;
std::vector<unsigned int> vTwoInverses;

static unsigned int int_invert(unsigned int a, unsigned int nPrime);



void GeneratePrimeTable(unsigned int nSieveSize)
{
    const unsigned int nPrimeTableLimit = nSieveSize ;
    vPrimes.clear();
	// Generate prime table using sieve of Eratosthenes
    std::vector<bool> vfComposite (nPrimeTableLimit, false);
	for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
	{
		if (vfComposite[nFactor])
			continue;
		for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
			vfComposite[nComposite] = true;
	}
	for (unsigned int n = 2; n < nPrimeTableLimit; n++)
		if (!vfComposite[n])
            vPrimes.push_back(n);
    printf("GeneratePrimeTable() : prime table [1, %d] generated with %lu primes\n", nPrimeTableLimit, vPrimes.size());
	vPrimesSize = vPrimes.size();
    
    const unsigned int nPrimes = vPrimes.size();
    vTwoInverses = std::vector<unsigned int> (nPrimes, 0);
    for (unsigned int nPrimeSeq = 1; nPrimeSeq < nPrimes; nPrimeSeq++)
		{
        vTwoInverses[nPrimeSeq] = int_invert(2, vPrimes[nPrimeSeq]);
	}
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


// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTestFast(const mpz_class& n, unsigned int& nLength, CPrimalityTestParams& testParams, bool fFastDiv = false, bool fFastFail = false)
{
    // Faster GMP version
    mpz_t& mpzE = testParams.mpzE;
    mpz_t& mpzR = testParams.mpzR;

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

    mpz_sub_ui(mpzE, n.get_mpz_t(), 1);
    mpz_powm(mpzR, mpzTwo.get_mpz_t(), mpzE, n.get_mpz_t());
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
static bool ProbableCunninghamChainTestFast(const mpz_class& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength, CPrimalityTestParams& testParams)
{
    nProbableChainLength = 0;

    // Fermat test for n first
    if (!FermatProbablePrimalityTestFast(n, nProbableChainLength, testParams, true, true))
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
static bool ProbablePrimeChainTestFast(const mpz_class& mpzPrimeChainOrigin, CPrimalityTestParams& testParams)
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

    // Test for Cunningham Chain of first kind
    mpzOriginMinusOne = mpzPrimeChainOrigin - 1;
    ProbableCunninghamChainTestFast(mpzOriginMinusOne, true, fFermatTest, nChainLengthCunningham1, testParams);
    // Test for Cunningham Chain of second kind
    mpzOriginPlusOne = mpzPrimeChainOrigin + 1;
    ProbableCunninghamChainTestFast(mpzOriginPlusOne, false, fFermatTest, nChainLengthCunningham2, testParams);

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

//// Sieve for mining
//boost::thread_specific_ptr<CSieveOfEratosthenes> psieve;

// Mine probable prime chain of form: n = h * p# +/- 1
bool MineProbablePrimeChain(CSieveOfEratosthenes** psieve, primecoinBlock_t* block, mpz_class& mpzFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, 
							unsigned int& nTests, unsigned int& nPrimesHit, sint32 threadIndex, mpz_class& mpzHash, unsigned int nPrimorialMultiplier)
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
		*psieve = new CSieveOfEratosthenes(nMaxSieveSize, block->nBits, mpzHash, mpzFixedMultiplier);
		(*psieve)->nSievePercentage = nSievePercentage;
		(*psieve)->Weave();
	}

	mpz_class mpzHashMultiplier = mpzHash * mpzFixedMultiplier;
	mpz_class mpzChainOrigin;

    // Determine the sequence number of the round primorial
    unsigned int nPrimorialSeq = 0;
    while (vPrimes[nPrimorialSeq + 1] <= nPrimorialMultiplier)
        nPrimorialSeq++;

    // Allocate GMP variables for primality tests
    CPrimalityTestParams testParams(block->nBits, nPrimorialSeq);
    // Compute parameters for fast div test
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

	//nStart = GetTickCount();
	//nCurrent = nStart;

	//uint32 timeStop = GetTickCount() + 25000;
	//int nTries = 0;
	//bool multipleShare = false;
	
	while ( nTests < 1200 && block->serverData.blockHeight == jhMiner_getCurrentWorkBlockHeight(block->threadIndex))
	{
		//nTries++;
		nTests++;
		if (!(*psieve)->GetNextCandidateMultiplier(nTriedMultiplier))
		{
			// power tests completed for the sieve
			delete *psieve;
			*psieve = NULL;
			fNewBlock = true; // notify caller to change nonce
			return false;
		}
				
		mpzChainOrigin = mpzHash * mpzFixedMultiplier * nTriedMultiplier;		
		nChainLengthCunningham1 = 0;
		nChainLengthCunningham2 = 0;
		nChainLengthBiTwin = 0;

		
		bool canSubmitAsShare = ProbablePrimeChainTestFast(mpzChainOrigin, testParams);
		//CBigNum bnChainOrigin;
		//bnChainOrigin.SetHex(mpzChainOrigin.get_str(16));
		//bool canSubmitAsShare = ProbablePrimeChainTestBN(bnChainOrigin, block->serverData.nBitsForShare, false, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin);
		
		nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);

		if (nProbableChainLength >= 0x3000000)
		{
			primeStats.nChainHit += pow(8, ((float)((double)nProbableChainLength  / (double)0x1000000))-7.0);
			//primeStats.nChainHit += pow(8, floor((float)((double)nProbableChainLength  / (double)0x1000000)) - 7);
			if (nProbableChainLength >= 0x4000000)
			{
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
			//primeStats.shareFound = true;
			
			block->mpzPrimeChainMultiplier = mpzFixedMultiplier * nTriedMultiplier;
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

			float shareValue = pow(8, floor((float)((double)nProbableChainLength  / (double)0x1000000)) - 7);

			printf("%s - SHARE FOUND !!! (Th#: %u) ---  DIFF: %f - VAL: %f    %s    %s\n", 
				sNow, threadIndex, (float)((double)nProbableChainLength  / (double)0x1000000), shareValue, 
				nProbableChainLength >= 0x6000000 ? ">6":"", nProbableChainLength >= 0x7000000 ? ">7":"");

			// submit this share
			if (jhMiner_pushShare_primecoin(blockRawData, block))
				primeStats.foundShareCount ++;
			primeStats.fShareValue += shareValue;
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

/*
// Weave sieve for the next prime in table
// Return values:
//   True  - weaved another prime; nComposite - number of composites removed
//   False - sieve already completed
bool CSieveOfEratosthenes::WeaveFastAllBN()
{
	//CBigNum p = vPrimes[nPrimeSeq];
	// init some required bignums on the stack (no dynamic allocation at all)
	BIGNUM bn_p;
	BIGNUM bn_tmp;
	BIGNUM bn_fixedInverse;
	BIGNUM bn_twoInverse;
	BIGNUM bn_fixedMod;
	uint32 bignumData_p[0x200/4];
	uint32 bignumData_tmp[0x200/4];
	uint32 bignumData_fixedInverse[0x200/4];
	uint32 bignumData_twoInverse[0x200/4];
	uint32 bignumData_fixedMod[0x200/4];
	fastInitBignum(bn_p, bignumData_p);
	fastInitBignum(bn_tmp, bignumData_tmp);
	fastInitBignum(bn_fixedInverse, bignumData_fixedInverse);
	fastInitBignum(bn_twoInverse, bignumData_twoInverse);
	fastInitBignum(bn_fixedMod, bignumData_fixedMod);

	unsigned int nChainLength = TargetGetLength(nBits);
	unsigned int nChainLengthX2 = nChainLength*2;

	while( true )
	{
		if (nPrimeSeq >= vPrimesSize || vPrimes[nPrimeSeq] >= nSieveSize)
			return false;  // sieve has been completed

	
		BN_set_word(&bn_p, vPrimes[nPrimeSeq]);
		BN2_div(NULL, &bn_tmp, &bnFixedFactor, &bn_p);
		
		
		//if (bnFixedFactor % p == 0)
		if( BN_is_zero(&bn_tmp) )
		{
			// Nothing in the sieve is divisible by this prime
			nPrimeSeq++;
			continue;
		}

		//CBigNum p = CBigNum(vPrimes[nPrimeSeq]);
		// debug: Code is correct until here

		// Find the modulo inverse of fixed factor
		//if (!BN2_mod_inverse(&bn_fixedInverse, &bnFixedFactor, &bn_p, pctx))
		//	return error("CSieveOfEratosthenes::Weave(): BN_mod_inverse of fixed factor failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
		//

		//sint32 djf1 = single_modinv_ak(bn_tmp.d[0], vPrimes[nPrimeSeq]);
		//sint32 djf2 = int_invert(bn_tmp.d[0], vPrimes[nPrimeSeq]);
		//if( djf1 != djf2 )
		//	__debugbreak();

		BN_set_word(&bn_fixedInverse, single_modinv(bn_tmp.d[0], vPrimes[nPrimeSeq]));

		//CBigNum bnTwo = 2;
		//if (!BN_mod_inverse(&bn_twoInverse, &bn_constTwo, &bn_p, pctx))
		//	return error("CSieveOfEratosthenes::Weave(): BN_mod_inverse of 2 failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
		//if( BN_cmp(&bn_twoInverse, &CBigNum(vPrimesTwoInverse[nPrimeSeq])) )
		//	__debugbreak();
		
		////BN_set_word(&bn_twoInverse, vPrimesTwoInverse[nPrimeSeq]);

		uint64 pU64 = (uint64)vPrimes[nPrimeSeq];
		uint64 fixedInverseU64 = BN_get_word(&bn_fixedInverse);
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
					vfCompositeBiTwin[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			if (((nBiTwinSeq & 1u) == 0))
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeCunningham1[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			if (((nBiTwinSeq & 1u) == 1u))
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeCunningham2[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
		}

		nPrimeSeq++;
	}
	return false; // never reached
}
*/

/*
// Weave sieve for the next prime in table
// Return values:
//   True  - weaved another prime; nComposite - number of composites removed
//   False - sieve already completed
bool CSieveOfEratosthenes::WeaveFastAll()
{

	  // Faster GMP version
    const unsigned int nChainLength = TargetGetLength(nBits);
    unsigned int nPrimeSeq = 0;
    unsigned int vPrimesSize2 = vPrimesSize;

    // Keep all variables local for max performance
    unsigned int nSieveSize = this->nSieveSize;

    // Process only 10% of the primes
    // Most composites are still found
    vPrimesSize2 = (uint64)vPrimesSize2 *nSievePercentage / 100;

	
    mpz_t mpzFixedFactor; // fixed factor to derive the chain
    mpz_t mpzFixedFactorMod;
    mpz_t p;
    mpz_t mpzFixedInverse;
    //mpz_t mpzTwo;
    mpz_t mpzTwoInverse;


	unsigned long nFixedFactorMod;
    unsigned long nFixedInverse;
    unsigned long nTwoInverse;

	mpz_init_set(mpzFixedFactor, this->mpzFixedFactor.get_mpz_t());
    mpz_init(mpzFixedFactorMod);
    mpz_init(p);
    mpz_init(mpzFixedInverse);
    //mpz_init_set_ui(mpzTwo, 2);
    mpz_init(mpzTwoInverse);
	
	while( true )
	{
		unsigned int nPrime = vPrimes[nPrimeSeq];
		if (nPrimeSeq >= vPrimesSize2 || nPrime >= nSieveSize)
		{
			break;
			//return false;  // sieve has been completed

		}
		//BN_set_word(&bn_p, nPrime);
        //mpz_set_ui(p, nPrime);

		//BN_mod(&bn_tmp, &bnFixedFactor, &bn_p, pctx);
        nFixedFactorMod = mpz_tdiv_r_ui(mpzFixedFactorMod, mpzFixedFactor, nPrime);
        
        if (nFixedFactorMod == 0)
        {
            // Nothing in the sieve is divisible by this prime
            nPrimeSeq++;
            continue;
        }
        mpz_set_ui(p, nPrime);

		// Find the modulo inverse of fixed factor
        if (!mpz_invert(mpzFixedInverse, mpzFixedFactorMod, p))
            return error("CSieveOfEratosthenes::Weave(): mpz_invert of fixed factor failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
        //nFixedInverse = mpz_get_ui(mpzFixedInverse);
        if (!mpz_invert(mpzTwoInverse, mpzTwo, p))
            return error("CSieveOfEratosthenes::Weave(): mpz_invert of 2 failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
        //nTwoInverse = mpz_get_ui(mpzTwoInverse);
		
		uint64 pU64 = (uint64)vPrimes[nPrimeSeq];
		//uint64 fixedInverseU64 = BN_get_word(&bn_fixedInverse);
		uint64 fixedInverseU64 = mpz_get_ui(mpzFixedInverse);
		//uint64 twoInverseU64 = BN_get_word(&bn_twoInverse);
		uint64 twoInverseU64 = mpz_get_ui(mpzTwoInverse);
		// Weave the sieve for the prime		
		for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < 2 * nChainLength; nBiTwinSeq++)
		{

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
					vfCompositeBiTwin[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			if (((nBiTwinSeq & 1u) == 0))
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeCunningham1[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
			if (((nBiTwinSeq & 1u) == 1u))
				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)
					vfCompositeCunningham2[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);
		}
		nPrimeSeq++;
	}
	this->nPrimeSeq = nPrimeSeq;

	mpz_clear(mpzFixedFactor);
    mpz_clear(mpzFixedFactorMod);
    mpz_clear(p);
    mpz_clear(mpzFixedInverse);
    //mpz_clear(mpzTwo);
    mpz_clear(mpzTwoInverse);

	return false; // never reached
}
*/

static unsigned int int_invert(unsigned int a, unsigned int nPrime)
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

void CSieveOfEratosthenes::AddMultiplier(unsigned int *vMultipliers, const unsigned int nPrimeSeq, const unsigned int nSolvedMultiplier)
{
    // Eliminate duplicates
    for (unsigned int i = 0; i < nHalfChainLength; i++)
    {
        unsigned int nStoredMultiplier = vMultipliers[nPrimeSeq * nHalfChainLength + i];
        if (nStoredMultiplier == 0xFFFFFFFF || nStoredMultiplier == nSolvedMultiplier)
        {
            vMultipliers[nPrimeSeq * nHalfChainLength + i] = nSolvedMultiplier;
            break;
        }
    }
}


// Weave sieve for the next prime in table
// Return values:
//   True  - weaved another prime; nComposite - number of composites removed
//   False - sieve already completed
bool CSieveOfEratosthenes::Weave()
{
    // Faster GMP version
 
    // Keep all variables local for max performance
    const unsigned int nChainLength = this->nChainLength;
    const unsigned int nDoubleChainLength = this->nChainLength * 2;

    const unsigned int nHalfChainLength = this->nHalfChainLength;
//    CBlockIndex* pindexPrev = this->pindexPrev;
    const unsigned int nSieveSize = this->nSieveSize;
    const unsigned int nTotalPrimes = vPrimes.size();
	const unsigned int nTotalPrimesLessOne = nTotalPrimes-1;
    mpz_class mpzHash = this->mpzHash;
    mpz_class mpzFixedMultiplier = this->mpzFixedMultiplier;

    // Process only a set percentage of the primes
    // Most composites are still found
    const unsigned int nPrimes = (uint64)nTotalPrimes * nSievePercentage / 100;
    this->nPrimes = nPrimes;

    const unsigned int nMultiplierBytes = nPrimes * nHalfChainLength * sizeof(unsigned int);
    unsigned int *vCunningham1AMultipliers = (unsigned int *)malloc(nMultiplierBytes);
    unsigned int *vCunningham1BMultipliers = (unsigned int *)malloc(nMultiplierBytes);
    unsigned int *vCunningham2AMultipliers = (unsigned int *)malloc(nMultiplierBytes);
    unsigned int *vCunningham2BMultipliers = (unsigned int *)malloc(nMultiplierBytes);

    memset(vCunningham1AMultipliers, 0xFF, nMultiplierBytes);
    memset(vCunningham1BMultipliers, 0xFF, nMultiplierBytes);
    memset(vCunningham2AMultipliers, 0xFF, nMultiplierBytes);
    memset(vCunningham2BMultipliers, 0xFF, nMultiplierBytes);

    // bitsets that can be combined to obtain the final bitset of candidates
    unsigned long *vfCompositeCunningham1A = (unsigned long *)malloc(nCandidatesBytes);
    unsigned long *vfCompositeCunningham1B = (unsigned long *)malloc(nCandidatesBytes);
    unsigned long *vfCompositeCunningham2A = (unsigned long *)malloc(nCandidatesBytes);
    unsigned long *vfCompositeCunningham2B = (unsigned long *)malloc(nCandidatesBytes);

    unsigned long *vfCandidates = this->vfCandidates;

    // Check whether fixed multiplier fits in an unsigned long
    bool fUseLongForFixedMultiplier = mpzFixedMultiplier < ULONG_MAX;
    unsigned long nFixedMultiplier;
    mpz_class mpzFixedFactor;
    if (fUseLongForFixedMultiplier)
        nFixedMultiplier = mpzFixedMultiplier.get_ui();
    else
        mpzFixedFactor = mpzHash * mpzFixedMultiplier;

    unsigned int nCombinedEndSeq = 1;
    unsigned int nFixedFactorCombinedMod = 0;

    for (unsigned int nPrimeSeq = 1; nPrimeSeq < nPrimes; nPrimeSeq++)
    {
		// TODO: on new block stop
        //if (pindexPrev != pindexBest)
        //    break;  // new block
        unsigned int nPrime = vPrimes[nPrimeSeq];
        if (nPrimeSeq >= nCombinedEndSeq)
        {
            // Combine multiple primes to produce a big divisor
            unsigned int nPrimeCombined = 1;
            while (nPrimeCombined < UINT_MAX / vPrimes[nCombinedEndSeq] && nCombinedEndSeq < nTotalPrimesLessOne )
            {
                nPrimeCombined *= vPrimes[nCombinedEndSeq];
                nCombinedEndSeq++;
            }

            if (fUseLongForFixedMultiplier)
            {
                nFixedFactorCombinedMod = mpz_tdiv_ui(mpzHash.get_mpz_t(), nPrimeCombined);
                nFixedFactorCombinedMod = (uint64)nFixedFactorCombinedMod * (nFixedMultiplier % nPrimeCombined) % nPrimeCombined;
            }
            else
                nFixedFactorCombinedMod = mpz_tdiv_ui(mpzFixedFactor.get_mpz_t(), nPrimeCombined);
        }

        unsigned int nFixedFactorMod = nFixedFactorCombinedMod % nPrime;
        if (nFixedFactorMod == 0)
        {
            // Nothing in the sieve is divisible by this prime
            continue;
        }
        // Find the modulo inverse of fixed factor
        unsigned int nFixedInverse = int_invert(nFixedFactorMod, nPrime);
        if (!nFixedInverse)
            return error("CSieveOfEratosthenes::Weave(): int_invert of fixed factor failed for prime #%u=%u", nPrimeSeq, vPrimes[nPrimeSeq]);
        unsigned int nTwoInverse = vTwoInverses[nPrimeSeq];

        // Weave the sieve for the prime
        for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < nDoubleChainLength; nBiTwinSeq++)
        {
            // Find the first number that's divisible by this prime
            int nDelta = ((nBiTwinSeq % 2 == 0)? (-1) : 1);
            unsigned int nSolvedMultiplier = (uint64)nFixedInverse * (nPrime - nDelta) % nPrime;
            if (nBiTwinSeq % 2 == 1)
                nFixedInverse = (uint64)nFixedInverse * nTwoInverse % nPrime;

            if (nBiTwinSeq < nChainLength)
            {
                if (((nBiTwinSeq & 1u) == 0))
                    AddMultiplier(vCunningham1AMultipliers, nPrimeSeq, nSolvedMultiplier);
                else
                    AddMultiplier(vCunningham2AMultipliers, nPrimeSeq, nSolvedMultiplier);
            } else {
                if (((nBiTwinSeq & 1u) == 0))
                    AddMultiplier(vCunningham1BMultipliers, nPrimeSeq, nSolvedMultiplier);
                else
                    AddMultiplier(vCunningham2BMultipliers, nPrimeSeq, nSolvedMultiplier);
            }
        }
    }

    // Number of elements that are likely to fit in L1 cache
    const unsigned int nL1CacheElements = 200000;
    const unsigned int nArrayRounds = (nSieveSize + nL1CacheElements - 1) / nL1CacheElements;

    // Loop over each array one at a time for optimal L1 cache performance
    for (unsigned int j = 0; j < nArrayRounds; j++)
    {
        const unsigned int nMinMultiplier = nL1CacheElements * j;
        const unsigned int nMaxMultiplier = min(nL1CacheElements * (j + 1), nSieveSize);

		//TODO: stop on new block
		//if (pindexPrev != pindexBest)
        //    break;  // new block

        ProcessMultiplier(vfCompositeCunningham1A, nMinMultiplier, nMaxMultiplier, vPrimes, vCunningham1AMultipliers);
        ProcessMultiplier(vfCompositeCunningham1B, nMinMultiplier, nMaxMultiplier, vPrimes, vCunningham1BMultipliers);
        ProcessMultiplier(vfCompositeCunningham2A, nMinMultiplier, nMaxMultiplier, vPrimes, vCunningham2AMultipliers);
        ProcessMultiplier(vfCompositeCunningham2B, nMinMultiplier, nMaxMultiplier, vPrimes, vCunningham2BMultipliers);

        // Combine all the bitsets
        // vfCompositeCunningham1 = vfCompositeCunningham1A | vfCompositeCunningham1B
        // vfCompositeCunningham2 = vfCompositeCunningham2A | vfCompositeCunningham2B
        // vfCompositeBiTwin = vfCompositeCunningham1A | vfCompositeCunningham2A
        // vfCandidates = ~(vfCompositeCunningham1 & vfCompositeCunningham2 & vfCompositeBiTwin)
        {
            // Fast version
            const unsigned int nBytes = (nMaxMultiplier - nMinMultiplier + 7) / 8;
            unsigned long *lCandidates = (unsigned long *)vfCandidates + (nMinMultiplier / nWordBits);
            unsigned long *lCompositeCunningham1A = (unsigned long *)vfCompositeCunningham1A + (nMinMultiplier / nWordBits);
            unsigned long *lCompositeCunningham1B = (unsigned long *)vfCompositeCunningham1B + (nMinMultiplier / nWordBits);
            unsigned long *lCompositeCunningham2A = (unsigned long *)vfCompositeCunningham2A + (nMinMultiplier / nWordBits);
            unsigned long *lCompositeCunningham2B = (unsigned long *)vfCompositeCunningham2B + (nMinMultiplier / nWordBits);
            const unsigned int nLongs = (nBytes + sizeof(unsigned long) - 1) / sizeof(unsigned long);
            for (unsigned int i = 0; i < nLongs; i++)
            {
                lCandidates[i] = ~((lCompositeCunningham1A[i] | lCompositeCunningham1B[i]) &
                                (lCompositeCunningham2A[i] | lCompositeCunningham2B[i]) &
                                (lCompositeCunningham1A[i] | lCompositeCunningham2A[i]));
            }
        }
    }

    this->nPrimeSeq = nPrimes - 1;

    free(vfCompositeCunningham1A);
    free(vfCompositeCunningham1B);
    free(vfCompositeCunningham2A);
    free(vfCompositeCunningham2B);
    
    free(vCunningham1AMultipliers);
    free(vCunningham1BMultipliers);
    free(vCunningham2AMultipliers);
    free(vCunningham2BMultipliers);

    return false;
}

