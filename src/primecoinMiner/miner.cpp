#include"global.h"


bool MineProbablePrimeChain(CSieveOfEratosthenes** psieve, primecoinBlock_t* block, mpz_class& bnFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit);

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

	static const unsigned int nPrimorialHashFactor = 7;
	unsigned int nPrimorialMultiplierStart = 7;

	static int startFactorList[4] =
	{
		//7,7,7,7 <-- ??
		107,107,107,107
		// 9697,9697,9697,9697
	};
	nPrimorialMultiplierStart = startFactorList[(threadIndex&3)];

	unsigned int nPrimorialMultiplier = nPrimorialMultiplierStart;
	int64 nTimeExpected = 0;   // time expected to prime chain (micro-second)
	int64 nTimeExpectedPrev = 0; // time expected to prime chain last time
	bool fIncrementPrimorial = true; // increase or decrease primorial factor

	CSieveOfEratosthenes* psieve = NULL;

	primecoinBlock->nonce = 0;

	uint32 nTime = GetTickCount() + 1000*60;
	
	// note: originally a wanted to loop as long as (primecoinBlock->workDataHash != jhMiner_getCurrentWorkHash()) did not happen
	//		 but I noticed it might be smarter to just check if the blockHeight has changed, since that is what is really important
	nPrimorialMultiplier = nPrimorialMultiplierStart;
	uint32 loopCount = 0;

	mpz_class bnHashFactor;
	Primorial(nPrimorialHashFactor, bnHashFactor);

	time_t unixTimeStart;
	time(&unixTimeStart);
	uint32 nTimeRollStart = primecoinBlock->timestamp;

	while( GetTickCount() < nTime && primecoinBlock->serverData.blockHeight == jhMiner_getCurrentWorkBlockHeight(primecoinBlock->threadIndex) )
	{

		if( primecoinBlock->xptMode )
		{
			// when using x.pushthrough, roll time
			time_t unixTimeCurrent;
			time(&unixTimeCurrent);
			uint32 timeDif = unixTimeCurrent - unixTimeStart;
			uint32 newTimestamp = nTimeRollStart + timeDif;
			if( newTimestamp != primecoinBlock->timestamp )
			{
				primecoinBlock->timestamp = newTimestamp;
				primecoinBlock->nonce = 0;
				nPrimorialMultiplierStart = startFactorList[(threadIndex&3)];
			}
		}

		primecoinBlock_generateHeaderHash(primecoinBlock, primecoinBlock->blockHeaderHash.begin());
		//
		// Search
		//
		bool fNewBlock = true;
		unsigned int nTriedMultiplier = 0;
		uint256 phash = primecoinBlock->blockHeaderHash;
        mpz_class mpzHash;
        mpz_set_uint256(mpzHash.get_mpz_t(), phash);
		// Primecoin: try to find hash divisible by primorial
		while ((phash < hashBlockHeaderLimit || (mpzHash % bnHashFactor != 0)) && primecoinBlock->nonce < 0xffff0000)
		{
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
		mpz_class bnPrimorial;
		unsigned int nRoundTests = 0;
		unsigned int nRoundPrimesHit = 0;
		//int64 nPrimeTimerStart = GetTimeMicros();
		//if (nTimeExpected > nTimeExpectedPrev)
		//	fIncrementPrimorial = !fIncrementPrimorial;
		//nTimeExpectedPrev = nTimeExpected;
		//// Primecoin: dynamic adjustment of primorial multiplier
		//if (fIncrementPrimorial)
		//{
		//	if (!PrimeTableGetNextPrime(&nPrimorialMultiplier))
		//		error("PrimecoinMiner() : primorial increment overflow");
		//}
		//else if (nPrimorialMultiplier > nPrimorialHashFactor)
		//{
		//	if (!PrimeTableGetPreviousPrime(&nPrimorialMultiplier))
		//		error("PrimecoinMiner() : primorial decrement overflow");
		//}

		//if( loopCount > 0 )
		//{
		//	primecoinBlock->nonce++;
		///*	if (!PrimeTableGetNextPrime(&nPrimorialMultiplier))
		//		error("PrimecoinMiner() : primorial increment overflow");*/
		//}

		Primorial(nPrimorialMultiplier, bnPrimorial);

		unsigned int nTests = 0;
		unsigned int nPrimesHit = 0;
		

		mpz_class bnMultiplierMin = bnPrimeMin * bnHashFactor / mpzHash + 2;
		while (bnPrimorial < bnMultiplierMin )
		{
			if (!PrimeTableGetNextPrime(&nPrimorialMultiplier))
				error("PrimecoinMiner() : primorial minimum overflow");
			Primorial(nPrimorialMultiplier, bnPrimorial);
		}
		mpz_class bnFixedMultiplier;
		if(bnPrimorial > bnHashFactor){
			bnFixedMultiplier = bnPrimorial / bnHashFactor;
		}else{
			bnFixedMultiplier = 1;
		}
		//printf("fixedMultiplier: %d nPrimorialMultiplier: %d\n", BN_get_word(&bnFixedMultiplier), nPrimorialMultiplier);
		// Primecoin: mine for prime chain
		unsigned int nProbableChainLength;
		if (MineProbablePrimeChain(&psieve, primecoinBlock, bnFixedMultiplier, fNewBlock, nTriedMultiplier, nProbableChainLength, nTests, nPrimesHit))
		{
			// do nothing here, share is already submitted in MineProbablePrimeChain()
			break;
		}
		//psieve = NULL;
		nRoundTests += nTests;
		nRoundPrimesHit += nPrimesHit;
		// added this
		/*if( nPrimorialMultiplier >= 800 )
		{
			primecoinBlock->nonce++;
			nPrimorialMultiplier = nPrimorialMultiplierStart;
		}*/

		//if( nPrimorialMultiplier >= 800 )
		//{
		//	primecoinBlock->nonce++;
		//	nPrimorialMultiplier = nPrimorialMultiplierStart;
		//}

		//if( primecoinBlock->nonce >= 0x100 )
		//{
		//	printf("Base reset\n");
		//	primecoinBlock->nonce = 0;
		//	nPrimorialMultiplier = nPrimorialMultiplierStart;
		//}

		primecoinBlock->nonce++;
		loopCount++;
	}
	
}
