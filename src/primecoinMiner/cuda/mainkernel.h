#define MAX_CANDIDATES 960

struct cudaCandidate {
    char strChainOrigin[512];
    char strPrimeChainMultiplier[512];
    unsigned int blocknBits;
    unsigned int nChainLengthCunningham1;
    unsigned int nChainLengthCunningham2;
    unsigned int nChainLengthBiTwin;
};

void runCandidateSearchKernel(cudaCandidate *candidates, char *result, unsigned int num_candidates);
