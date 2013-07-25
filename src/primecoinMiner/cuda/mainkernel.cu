#include <stdio.h>

#include "mainkernel.h"
#include "mpz.h"    // multiple precision cuda code
#include "cuda_string.h"

//__device__ mpz_t mpzTemp;

#define mpz_clear mpz_destroy
#define mpz_cmp mpz_compare
#define mpz_mul mpz_mult
#define mpz_powm mpz_powmod

//copied constants from prime.h

static const unsigned int nFractionalBits = 24;
static const unsigned int TARGET_FRACTIONAL_MASK = (1u<<nFractionalBits) - 1;
static const unsigned int TARGET_LENGTH_MASK = ~TARGET_FRACTIONAL_MASK;
//static const uint64 nFractionalDifficultyMax = (1llu << (nFractionalBits + 32));
//static const uint64 nFractionalDifficultyMin = (1llu << 32);
//static const uint64 nFractionalDifficultyThreshold = (1llu << (8 + 32));
//static const unsigned int nWorkTransitionRatio = 32;

//end copy

//mpz_div(mpz_t *q, mpz_t *r, mpz_t *n, mpz_t *d)


//extra mpz_functions (quick and dirty...)
__device__ inline void mpz_tdiv_q(mpz_t *ROP, mpz_t *OP1, mpz_t *OP2)
{
    mpz_t mpzTemp;
    mpz_init(&mpzTemp);
    mpz_div(ROP,&mpzTemp,OP1,OP2);
    mpz_destroy(&mpzTemp);
}

__device__ inline void mpz_tdiv_r(mpz_t *ROP, mpz_t *OP1, mpz_t *OP2)
{
    mpz_t mpzTemp;
    mpz_init(&mpzTemp);
    mpz_div(&mpzTemp,ROP,OP1,OP2);
    mpz_destroy(&mpzTemp);
}

__device__ inline unsigned int mpz_get_ui(mpz_t *OP)
{
    return OP->digits[0];
}

//Set product to multiplicator times 2 raised to exponent_of_2. This operation can also be defined as a left shift, exponent_of_2 steps.
__device__ inline void mpz_mul_2exp (mpz_t *product, mpz_t *multiplicator, unsigned long int exponent_of_2)
{
    mpz_t mpzTemp;
    mpz_init(&mpzTemp);
    mpz_set_ui(&mpzTemp,2);
    unsigned int limit = exponent_of_2;
    //well this is ugly
    for(unsigned int i=0; i < limit; i++)
    	mpz_bit_lshift(&mpzTemp);

    mpz_mul(product,multiplicator,&mpzTemp);
    mpz_destroy(&mpzTemp);
}


//end extra mpz

__device__ bool devTargetSetLength(unsigned int nLength, unsigned int& nBits)
{
    if (nLength >= 0xff)
    {
        //printf("[CUDA] error TargetSetLength() : invalid length=%u\n", nLength);
	return false;
    }
    nBits &= TARGET_FRACTIONAL_MASK;
    nBits |= (nLength << nFractionalBits);
    return true;
}

__device__ unsigned int devTargetGetLength(unsigned int nBits)
{
    return ((nBits & TARGET_LENGTH_MASK) >> nFractionalBits);
}

__device__ unsigned int devTargetFromInt(unsigned int nLength)
{
    return (nLength << nFractionalBits);
}

__device__ void devTargetIncrementLength(unsigned int& nBits)
{
    nBits += (1 << nFractionalBits);
}

// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite; set fractional length in the nLength output
__device__ bool devFermatProbablePrimalityTest(mpz_t &mpzN, unsigned int& nLength)
{
    mpz_t mpzOne;
    mpz_t mpzTwo;
    //mpz_t mpzEight;

    //TODO: generate constants in a different kernel
    mpz_init(&mpzOne);
    mpz_set_ui(&mpzOne,1);	

    mpz_init(&mpzTwo);
    mpz_set_ui(&mpzTwo,2);

    //mpz_init(&mpzEight);
    //mpz_set_ui(&mpzEight,8);

    // Faster GMP version
    
    //mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    
    //mpz_init_set(mpzN, n.get_mpz_t());

    //e = n -1

    mpz_init(&mpzE);
    mpz_sub(&mpzE, &mpzN, &mpzOne);
    mpz_init(&mpzR);

    //BN_mod_exp(&r, &a, &e, &n);
	// r = 2^(n-1) & n
    mpz_powm(&mpzR, &mpzTwo, &mpzE, &mpzN);

    mpz_destroy(&mpzOne);
    mpz_destroy(&mpzTwo);

    if (mpz_cmp(&mpzR, &mpzOne) == 0)
    {
        mpz_clear(&mpzN);
        mpz_clear(&mpzE);
        mpz_clear(&mpzR);
        
        //printf("[CUDA] Fermat test true\n");
        return true;
    }
    // Failed Fermat test, calculate fractional length
    mpz_sub(&mpzE, &mpzN, &mpzR);
    mpz_mul_2exp(&mpzR, &mpzE, nFractionalBits);
    mpz_tdiv_q(&mpzE, &mpzR, &mpzN);

    unsigned int nFractionalLength = mpz_get_ui(&mpzE);
    mpz_clear(&mpzN);
    mpz_clear(&mpzE);
    mpz_clear(&mpzR);

    if (nFractionalLength >= (1 << nFractionalBits))
    {
	//printf("[CUDA] Error FermatProbablePrimalityTest() : fractional assert : nFractionalLength:%i nFractionalBits:%i\n", nFractionalLength, nFractionalBits);
        return false;
    }

    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;
}

//this version prints results for thread 0
__device__ bool devFermatProbablePrimalityTestWithPrint(mpz_t &mpzN, unsigned int& nLength, unsigned int index)
{
    bool prime = false;

    mpz_t mpzOne;
    mpz_t mpzTwo;
    //mpz_t mpzEight;

    //TODO: generate constants in a different kernel
    mpz_init(&mpzOne);
    mpz_set_ui(&mpzOne,1);	

    mpz_init(&mpzTwo);
    mpz_set_ui(&mpzTwo,2);

    //mpz_init(&mpzEight);
    //mpz_set_ui(&mpzEight,8);

    // Faster GMP version
    
    //mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    
    //mpz_init_set(mpzN, n.get_mpz_t());

    //e = n -1

    mpz_init(&mpzE);
    mpz_sub(&mpzE, &mpzN, &mpzOne);

    if(index == 0)
    {
	//printf("[0] N is: ");
	mpz_print(&mpzN);
	//printf("[0] E is: ");
	mpz_print(&mpzE);
    }

    mpz_init(&mpzR);

    //BN_mod_exp(&r, &a, &e, &n);
    mpz_powm(&mpzR, &mpzTwo, &mpzE, &mpzN);

    if(index == 0)
    {
	//printf("[0] R is: ");
	mpz_print(&mpzR);
    }

    mpz_destroy(&mpzOne);
    mpz_destroy(&mpzTwo);

    if (mpz_cmp(&mpzR, &mpzOne) == 0)
    {
	prime = true;  
	//if(index == 0)      
        	//printf("[0] Fermat test true\n");
    }

    mpz_clear(&mpzN);
    mpz_clear(&mpzE);
    mpz_clear(&mpzR);

    return prime;
    // Failed Fermat test, calculate fractional length
    /*mpz_sub(&mpzE, &mpzN, &mpzR);
    mpz_mul_2exp(&mpzR, &mpzE, nFractionalBits);
    mpz_tdiv_q(&mpzE, &mpzR, &mpzN);

    unsigned int nFractionalLength = mpz_get_ui(&mpzE);
    mpz_clear(&mpzN);
    mpz_clear(&mpzE);
    mpz_clear(&mpzR);

    if (nFractionalLength >= (1 << nFractionalBits))
    {
	if(index==0)
		//printf("[CUDA] Error FermatProbablePrimalityTest() : fractional assert : nFractionalLength:%i nFractionalBits:%i\n", nFractionalLength, nFractionalBits);
        return false;
    }

    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;*/
}

// Test probable primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
// fSophieGermain:
//   true:  n = 2p+1, p prime, aka Cunningham Chain of first kind
//   false: n = 2p-1, p prime, aka Cunningham Chain of second kind
// Return values
//   true: n is probable prime
//   false: n is composite; set fractional length in the nLength output
__device__ bool devEulerLagrangeLifchitzPrimalityTest(mpz_t &mpzN, bool fSophieGermain, unsigned int& nLength)
{

    mpz_t mpzOne;
    mpz_t mpzTwo;
    //mpz_t mpzEight;

    //TODO: generate constants in a different kernel
    mpz_init(&mpzOne);
    mpz_set_ui(&mpzOne,1);	

    mpz_init(&mpzTwo);
    mpz_set_ui(&mpzTwo,2);

    //mpz_init(&mpzEight);
    //mpz_set_ui(&mpzEight,8);

    // Faster GMP version
    //mpz_t mpzN;
    mpz_t mpzE;
    mpz_t mpzR;
    mpz_t temp;

    mpz_init(&temp);    

    mpz_init(&mpzE);
    mpz_sub(&mpzE, &mpzN, &mpzOne);
 
    //mpz_set(&temp,&mpzE);

   //e = (n - 1) >> 1;
    //from hp4: mpz_tdiv_q_2exp(&mpzE, &mpzE, 1);
    mpz_tdiv_q(&temp,&mpzE,&mpzTwo);
    mpz_set(&mpzE,&temp);

    mpz_destroy(&temp);

    mpz_init(&mpzR);
    mpz_powm(&mpzR, &mpzTwo, &mpzE, &mpzN);
   
    //nMod8 = n % 8; 
    //mpz_t mpzNMod8;
    //mpz_init(&mpzNMod8);
    //mpz_tdiv_r(&mpzNMod8,&mpzN, &mpzEight);
    unsigned int nMod8 = mpz_get_ui(&mpzN) % 8;    
    //mpz_destroy(&mpzNMod8);

    bool fPassedTest = false;
    if (fSophieGermain && (nMod8 == 7)) // Euler & Lagrange
        fPassedTest = !mpz_cmp(&mpzR, &mpzOne);
    else if (fSophieGermain && (nMod8 == 3)) // Lifchitz
    {
        mpz_t mpzRplusOne;
        mpz_init(&mpzRplusOne);
        mpz_add(&mpzRplusOne, &mpzR, &mpzOne);
        fPassedTest = !mpz_cmp(&mpzRplusOne, &mpzN);
        mpz_clear(&mpzRplusOne);
    }
    else if ((!fSophieGermain) && (nMod8 == 5)) // Lifchitz
    {
        mpz_t mpzRplusOne;
        mpz_init(&mpzRplusOne);
        mpz_add(&mpzRplusOne, &mpzR, &mpzOne);
        fPassedTest = !mpz_cmp(&mpzRplusOne, &mpzN);
        mpz_clear(&mpzRplusOne);
    }
    else if ((!fSophieGermain) && (nMod8 == 1)) // LifChitz
    {
        fPassedTest = !mpz_cmp(&mpzR, &mpzOne);
    }
    else
    {
        mpz_clear(&mpzN);
        mpz_clear(&mpzE);
        mpz_clear(&mpzR);
        mpz_destroy(&mpzOne);
        mpz_destroy(&mpzTwo);
        //printf("[CUDA] Error in EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8, (fSophieGermain? "first kind" : "second kind"));
        return false;
    }
    
    if (fPassedTest)
    {
        mpz_clear(&mpzN);
        mpz_clear(&mpzE);
        mpz_clear(&mpzR);
	mpz_destroy(&mpzOne);
        mpz_destroy(&mpzTwo);
        return true;
    }
    
    // Failed test, calculate fractional length
    //TODO: RCOPY
    mpz_mul(&mpzE, &mpzR, &mpzR);
    mpz_tdiv_r(&mpzR, &mpzE, &mpzN); // derive Fermat test remainder

    mpz_sub(&mpzE, &mpzN, &mpzR);
    mpz_mul_2exp(&mpzR, &mpzE, nFractionalBits);
    mpz_tdiv_q(&mpzE, &mpzR, &mpzN);

    //Todo: implement mpz_get_ui
    unsigned int nFractionalLength = mpz_get_ui(&mpzE);
    mpz_clear(&mpzN);
    mpz_clear(&mpzE);
    mpz_clear(&mpzR);
    mpz_destroy(&mpzOne);
    mpz_destroy(&mpzTwo);
    
    if (nFractionalLength >= (1 << nFractionalBits))
    {
        //printf("[CUDA] error EulerLagrangeLifchitzPrimalityTest() : fractional assert");
        return false;
    }
    nLength = (nLength & TARGET_LENGTH_MASK) | nFractionalLength;
    return false;
}



// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (length at least 2)
//   false - Not Cunningham Chain
__device__ bool devProbableCunninghamChainTest(mpz_t &n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    //mpz_class N = n;

    mpz_t N;
    mpz_init(&N);

    mpz_t N_copy;
    mpz_init(&N_copy);

    mpz_set(&N,&n);    

    // Fermat test for n first
    if (!devFermatProbablePrimalityTest(N, nProbableChainLength))
        return false;

    //printf("[CUDA ] N is prime!\n");

    // Euler-Lagrange-Lifchitz test for the following numbers in chain
    while (true)
    {
        devTargetIncrementLength(nProbableChainLength);
	//N = N + N or N *=2
	mpz_set(&N_copy,&N);  
        mpz_mult_u(&N,&N_copy,2);
	// N+ = (fSophieGermain? 1 : (-1))
	mpz_addeq_i(&N,(fSophieGermain? 1 : (-1)));
        if (fFermatTest)
        {
            if (!devFermatProbablePrimalityTest(N, nProbableChainLength))
                break;
        }
        else
        {
            if (!devEulerLagrangeLifchitzPrimalityTest(N, fSophieGermain, nProbableChainLength))
                break;
        }
    }

    mpz_destroy(&N);
    mpz_destroy(&N_copy);

    return (devTargetGetLength(nProbableChainLength) >= 2);
}

// Test probable prime chain for: nOrigin
// Return value:
//   true - Probable prime chain found (one of nChainLength meeting target)
//   false - prime chain too short (none of nChainLength meeting target)
__device__ bool devProbablePrimeChainTest(mpz_t &mpzPrimeChainOrigin, unsigned int nBits, bool fFermatTest, unsigned int& nChainLengthCunningham1, unsigned int& nChainLengthCunningham2, unsigned int& nChainLengthBiTwin)
{
    mpz_t mpzOne;
    mpz_init(&mpzOne);
    mpz_set_ui(&mpzOne,1);

    nChainLengthCunningham1 = 0;
    nChainLengthCunningham2 = 0;
    nChainLengthBiTwin = 0;

    mpz_t mpzPrimeChainOriginMinusOne;
    mpz_t mpzPrimeChainOriginPlusOne;

    mpz_init(&mpzPrimeChainOriginMinusOne);
    mpz_init(&mpzPrimeChainOriginPlusOne);

    mpz_add(&mpzPrimeChainOriginPlusOne,&mpzPrimeChainOrigin,&mpzOne);
    mpz_sub(&mpzPrimeChainOriginMinusOne,&mpzPrimeChainOrigin,&mpzOne);

    // Test for Cunningham Chain of first kind
    devProbableCunninghamChainTest(mpzPrimeChainOriginMinusOne, true, fFermatTest, nChainLengthCunningham1);
    // Test for Cunningham Chain of second kind
    devProbableCunninghamChainTest(mpzPrimeChainOriginPlusOne, false, fFermatTest, nChainLengthCunningham2);
    // Figure out BiTwin Chain length
    // BiTwin Chain allows a single prime at the end for odd length chain
    nChainLengthBiTwin =
        (devTargetGetLength(nChainLengthCunningham1) > devTargetGetLength(nChainLengthCunningham2))?
            (nChainLengthCunningham2 + devTargetFromInt(devTargetGetLength(nChainLengthCunningham2)+1)) :
            (nChainLengthCunningham1 + devTargetFromInt(devTargetGetLength(nChainLengthCunningham1)));

    mpz_destroy(&mpzPrimeChainOriginMinusOne);
    mpz_destroy(&mpzPrimeChainOriginPlusOne);
    mpz_destroy(&mpzOne);

    return (nChainLengthCunningham1 >= nBits || nChainLengthCunningham2 >= nBits || nChainLengthBiTwin >= nBits);
}

__global__ void runCandidateSearch(cudaCandidate *candidates, char *result, unsigned int num_candidates)
{
    mpz_t mpzOne;
    mpz_init(&mpzOne);
    mpz_set_ui(&mpzOne,1);

    /*mpz_t mpzTwo;
    mpz_init(&mpzTwo);
    mpz_set_ui(&mpzTwo,2);*/

    mpz_t mpzN1;
    mpz_init(&mpzN1);

    mpz_t mpzN2;
    mpz_init(&mpzN2);
    //mpz_set_ui(&mpzOne,1);

	unsigned int index = threadIdx.x + blockIdx.x * blockDim.x;
	//check bounds
	if (index < num_candidates)
	{
		if(index==0)
		{
			//printf("[0] start! \n");
			//printf("sizeof(struct) = %i\n",sizeof(cudaCandidate));		
		}

		cudaCandidate candidate = candidates[index];

		//if(index==0)
			//printf("[0] candidate is %s\n",candidate.strChainOrigin);

		mpz_t mpzChainOrigin;
		mpz_init(&mpzChainOrigin);
	
		//FIXME: mpz_set_str doesnt work on the device right now
		mpz_set_str(&mpzChainOrigin,candidate.strChainOrigin, index);

		if(index==0)
		{
			//printf("[0] chain origin digits[0]: %x\n", mpzChainOrigin.digits[0]);

			//printf("[0] chain origin:");
			mpz_print(&mpzChainOrigin);
		}

		mpz_add(&mpzN1,&mpzChainOrigin,&mpzOne);
		mpz_sub(&mpzN2,&mpzChainOrigin,&mpzOne);

		unsigned int nLength=0;

		char testresult = 0x00;

		//test for chain of length two
		if(devFermatProbablePrimalityTestWithPrint(mpzN1, nLength, index) || devFermatProbablePrimalityTestWithPrint(mpzN2, nLength, index))
		{
			/*mpz_t mpzN1_copy;
    			mpz_init(&mpzN1_copy);
			mpz_set(&mpzN1_copy,&mpzN1);    

			mpz_t mpzN2_copy;
    			mpz_init(&mpzN2_copy);
			mpz_set(&mpzN2_copy,&mpzN2);  

        		mpz_mult_u(&mpzN1,&mpzN1_copy,2);
			mpz_mult_u(&mpzN2,&mpzN2_copy,2);

			mpz_addeq_i(&mpzN1,1);
			mpz_addeq_i(&mpzN2,-1);

			if(devFermatProbablePrimalityTestWithPrint(mpzN1, nLength, index) || devFermatProbablePrimalityTestWithPrint(mpzN2, nLength, index))
			{*/
				testresult = 0x01;
			//}
		}
		
		result[index] = testresult;
		

		if(index==0)
			//printf("[0] after fermat test\n");


        	/*if(index==0)
			//printf("[0] loaded\n");

		if (devProbablePrimeChainTest(mpzChainOrigin, candidate.blocknBits, false, candidate.nChainLengthCunningham1, candidate.nChainLengthCunningham2, candidate.nChainLengthBiTwin))
		{
			//printf("[CUDA] Found probable chain!\n");
			result[index] = 0x01;
		}else
		{
			result[index] = 0x00;
        	}*/

		mpz_destroy(&mpzChainOrigin);

	}

}

void runCandidateSearchKernel(cudaCandidate *candidates, char *result, unsigned int num_candidates)
{
	//TODO: make gridsize dynamic
	runCandidateSearch<<< 24 , 40>>>(candidates, result, num_candidates);

}
