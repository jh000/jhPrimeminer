#include"global.h"

CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);

CL_API_ENTRY cl_context (CL_API_CALL
						 *clCreateContext)(const cl_context_properties * /* properties */,
						 cl_uint                 /* num_devices */,
						 const cl_device_id *    /* devices */,
						 void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
						 void *                  /* user_data */,
						 cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_command_queue (CL_API_CALL
							   *clCreateCommandQueue)(cl_context                     /* context */, 
							   cl_device_id                   /* device */, 
							   cl_command_queue_properties    /* properties */,
							   cl_int *                       /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clGetPlatformIDs)(cl_uint          /* num_entries */,
					 cl_platform_id * /* platforms */,
					 cl_uint *        /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_mem (CL_API_CALL
					 *clCreateBuffer)(cl_context   /* context */,
					 cl_mem_flags /* flags */,
					 size_t       /* size */,
					 void *       /* host_ptr */,
					 cl_int *     /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_program (CL_API_CALL
						 *clCreateProgramWithSource)(cl_context        /* context */,
						 cl_uint           /* count */,
						 const char **     /* strings */,
						 const size_t *    /* lengths */,
						 cl_int *          /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clBuildProgram)(cl_program           /* program */,
					 cl_uint              /* num_devices */,
					 const cl_device_id * /* device_list */,
					 const char *         /* options */, 
					 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
					 void *               /* user_data */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clGetProgramBuildInfo)(cl_program            /* program */,
					 cl_device_id          /* device */,
					 cl_program_build_info /* param_name */,
					 size_t                /* param_value_size */,
					 void *                /* param_value */,
					 size_t *              /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_kernel (CL_API_CALL
						*clCreateKernel)(cl_program      /* program */,
						const char *    /* kernel_name */,
						cl_int *        /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clSetKernelArg)(cl_kernel    /* kernel */,
					 cl_uint      /* arg_index */,
					 size_t       /* arg_size */,
					 const void * /* arg_value */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
					 cl_kernel        /* kernel */,
					 cl_uint          /* work_dim */,
					 const size_t *   /* global_work_offset */,
					 const size_t *   /* global_work_size */,
					 const size_t *   /* local_work_size */,
					 cl_uint          /* num_events_in_wait_list */,
					 const cl_event * /* event_wait_list */,
					 cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
					 cl_mem              /* buffer */,
					 cl_bool             /* blocking_read */,
					 size_t              /* offset */,
					 size_t              /* size */, 
					 void *              /* ptr */,
					 cl_uint             /* num_events_in_wait_list */,
					 const cl_event *    /* event_wait_list */,
					 cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clEnqueueWriteBuffer)(cl_command_queue   /* command_queue */, 
					 cl_mem             /* buffer */, 
					 cl_bool            /* blocking_write */, 
					 size_t             /* offset */, 
					 size_t             /* size */, 
					 const void *       /* ptr */, 
					 cl_uint            /* num_events_in_wait_list */, 
					 const cl_event *   /* event_wait_list */, 
					 cl_event *         /* event */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clGetKernelWorkGroupInfo)(cl_kernel                  /* kernel */,
					 cl_device_id               /* device */,
					 cl_kernel_work_group_info  /* param_name */,
					 size_t                     /* param_value_size */,
					 void *                     /* param_value */,
					 size_t *                   /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clGetDeviceInfo)(cl_device_id    /* device */,
					 cl_device_info  /* param_name */, 
					 size_t          /* param_value_size */, 
					 void *          /* param_value */,
					 size_t *        /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

// global OpenCL stuff
cl_context openCL_context;
cl_command_queue openCL_queue;
cl_device_id openCL_device;

typedef struct  
{
	bool kernelInited;
	// constants
	sint32 numSegments;
	//sint32 sizeSegment;
	sint32 sieveMaskSize;
	// memory buffers
	//uint8* mem_maskC1;
	//cl_mem buffer_maskC1;
	//uint8* mem_maskC2;
	//cl_mem buffer_maskC2;
	//uint8* mem_maskBT;
	//cl_mem buffer_maskBT;
	cl_int nCLPrimesToSieve;
	uint32* buffer_candidateList;
	cl_mem mem_candidateList;
	uint32 candidatesPerT;

	uint32* mem_primes;
	cl_mem buffer_primes;
	uint32* mem_twoInverse;
	cl_mem buffer_twoInverse;
	uint32* mem_primeBase;
	cl_mem buffer_primeBase;
	// kernels
	cl_kernel kernel_sievePrimes;
	// number of workgroups (number of how many small sieves we generate for each kernel execution)
	uint32 workgroupNum;
	// device and kernel info
	uint32 device_localMemorySize;
	uint32 device_preferredWorkSizeMultiple;
}gpuCL_t;

gpuCL_t gpuCL = {0};

void openCL_init()
{
	printf("Init OpenCL...\n");
	HMODULE openCLModule = LoadLibraryA("opencl.dll");
	if( openCLModule == NULL )
	{
		printf("Failed to load OpenCL :(\n");
		exit(-1);
	}
	
	*(void**)&clGetDeviceIDs = (void*)GetProcAddress(openCLModule, "clGetDeviceIDs");
	*(void**)&clCreateContext = (void*)GetProcAddress(openCLModule, "clCreateContext");
	*(void**)&clCreateCommandQueue = (void*)GetProcAddress(openCLModule, "clCreateCommandQueue");
	*(void**)&clGetPlatformIDs = (void*)GetProcAddress(openCLModule, "clGetPlatformIDs");
	*(void**)&clCreateBuffer = (void*)GetProcAddress(openCLModule, "clCreateBuffer");
	*(void**)&clCreateProgramWithSource = (void*)GetProcAddress(openCLModule, "clCreateProgramWithSource");
	*(void**)&clBuildProgram = (void*)GetProcAddress(openCLModule, "clBuildProgram");
	*(void**)&clGetProgramBuildInfo = (void*)GetProcAddress(openCLModule, "clGetProgramBuildInfo");
	*(void**)&clCreateKernel = (void*)GetProcAddress(openCLModule, "clCreateKernel");
	*(void**)&clSetKernelArg = (void*)GetProcAddress(openCLModule, "clSetKernelArg");
	*(void**)&clEnqueueNDRangeKernel = (void*)GetProcAddress(openCLModule, "clEnqueueNDRangeKernel");
	*(void**)&clEnqueueReadBuffer = (void*)GetProcAddress(openCLModule, "clEnqueueReadBuffer");
	*(void**)&clEnqueueWriteBuffer = (void*)GetProcAddress(openCLModule, "clEnqueueWriteBuffer");
	*(void**)&clGetKernelWorkGroupInfo = (void*)GetProcAddress(openCLModule, "clGetKernelWorkGroupInfo");
	*(void**)&clGetDeviceInfo = (void*)GetProcAddress(openCLModule, "clGetDeviceInfo");

	


	printf("Creating OpenCL context...\n");
	openCL_createContext();
}

void openCL_getDeviceInfo()
{
	// create an empty kernel
	char* source = "__kernel void jhp_ghostKernel(__global *a) {a[0] = 3;}";
	size_t src_size = fStrLen(source);
	cl_int clerr;
	cl_program program = clCreateProgramWithSource(openCL_context, 1, (const char**)&source, &src_size, &clerr);
	if(clerr != CL_SUCCESS)
		printf("Error creating OpenCL ghost kernel\n");
	clerr = clBuildProgram(program, 1, &openCL_device, NULL, NULL, NULL);
	if(clerr != CL_SUCCESS)
		printf("Error compiling OpenCL ghost kernel\n");
	char* build_log;
	size_t log_size;
	clGetProgramBuildInfo(program, openCL_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	build_log = (char*)malloc(log_size+1);
	memset(build_log, 0x00, log_size+1);
	clGetProgramBuildInfo(program, openCL_device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
	build_log[log_size] = '\0';
	puts(build_log);
	free(build_log);
	cl_kernel ghostKernel = clCreateKernel(program, "jhp_ghostKernel", &clerr);
	// no we can request some device and kernel info
	cl_ulong deviceLocalMemorySize = 0;
	clGetDeviceInfo(openCL_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &deviceLocalMemorySize, NULL);
	gpuCL.device_localMemorySize = deviceLocalMemorySize;
	size_t preferredWorkGroupMultiple = 0;
	clGetKernelWorkGroupInfo(ghostKernel, openCL_device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &preferredWorkGroupMultiple, NULL);
	gpuCL.device_preferredWorkSizeMultiple = preferredWorkGroupMultiple;
	// todo: Free ghostKernel

	// print some info
	printf("OpenCL device info:\n");
	printf("Local memory = %d bytes (%d sieve size)\n", gpuCL.device_localMemorySize, (gpuCL.device_localMemorySize*8/3));
	printf("Preferred work size multiple = %d\n", gpuCL.device_preferredWorkSizeMultiple);
}

void openCL_generateOrUpdateKernel(uint32 nBits, uint32 sieveSize)
{
	if( gpuCL.kernelInited )
		return;
	gpuCL.kernelInited = true;
	openCL_getDeviceInfo();

	fStr_t* fStr_src = fStr_alloc(1024*512);
	// init header

	fStr_append(fStr_src, "#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable\r\n");
	fStr_append(fStr_src, "#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable\r\n");
	fStr_append(fStr_src, "#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable\r\n");
	fStr_append(fStr_src, "#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable\r\n");

	cl_int clerr = 0;
	//gpuCL.numSegments = 64*4;
	//gpuCL.sizeSegment = 16*1024;
	gpuCL.workgroupNum = 1; // num of compute units
	gpuCL.numSegments = gpuCL.device_preferredWorkSizeMultiple*64;

	//256; --> We cant set this higher than 32 on NVIDIA because it causes the threads above 32 have different local memory
	sieveSize = gpuCL.device_localMemorySize*8/3; // todo: Replace with dynamic OpenCL shared memory size
	//gpuCL.sieveSizeSegment = sieveSize / gpuCL.numSegments;
	gpuCL.sieveMaskSize = sieveSize/8; // round down to avoid local memory overflow

	gpuCL.candidatesPerT = 16;
	gpuCL.buffer_candidateList = (uint32*)malloc(gpuCL.candidatesPerT*gpuCL.numSegments*gpuCL.workgroupNum*sizeof(uint32));
	gpuCL.mem_candidateList = clCreateBuffer(openCL_context, CL_MEM_WRITE_ONLY, gpuCL.candidatesPerT*gpuCL.numSegments*gpuCL.workgroupNum*sizeof(uint32), gpuCL.buffer_candidateList, &clerr);
	//gpuCL.mem_maskC1 = (uint8*)malloc(gpuCL.sieveMaskSize);
	//gpuCL.buffer_maskC1 = clCreateBuffer(openCL_context, CL_MEM_READ_WRITE, gpuCL.sieveMaskSize, gpuCL.mem_maskC1, &clerr);
	//gpuCL.mem_maskC2 = (uint8*)malloc(gpuCL.sieveMaskSize);
	//gpuCL.buffer_maskC2 = clCreateBuffer(openCL_context, CL_MEM_READ_WRITE, gpuCL.sieveMaskSize, gpuCL.mem_maskC2, &clerr);
	//gpuCL.mem_maskBT = (uint8*)malloc(gpuCL.sieveMaskSize);
	//gpuCL.buffer_maskBT = clCreateBuffer(openCL_context, CL_MEM_READ_WRITE, gpuCL.sieveMaskSize, gpuCL.mem_maskBT, &clerr);

	gpuCL.nCLPrimesToSieve = 300000;

	gpuCL.mem_primes = (uint32*)malloc(sizeof(uint32)*gpuCL.nCLPrimesToSieve);
	gpuCL.buffer_primes = clCreateBuffer(openCL_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(uint32)*gpuCL.nCLPrimesToSieve, gpuCL.mem_primes, &clerr);
	gpuCL.mem_twoInverse = (uint32*)malloc(sizeof(uint32)*gpuCL.nCLPrimesToSieve);
	gpuCL.buffer_twoInverse = clCreateBuffer(openCL_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(uint32)*gpuCL.nCLPrimesToSieve, gpuCL.mem_twoInverse, &clerr);
	gpuCL.mem_primeBase = (uint32*)malloc(sizeof(uint32)*gpuCL.nCLPrimesToSieve);
	gpuCL.buffer_primeBase = clCreateBuffer(openCL_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(uint32)*gpuCL.nCLPrimesToSieve, gpuCL.mem_primeBase, &clerr);


	fStr_appendFormatted(fStr_src, "__constant int nSieveSize = %d;\r\n", sieveSize);
	fStr_appendFormatted(fStr_src, "__constant int nChainLength = %d;\r\n", nBits>>24);

	// todo: Use spacing between prime multipliers to solve the congruency problem (e.g. mark each prime step that is idx*p*32+p*threadId
	fStr_appendFormatted(fStr_src, "__attribute__((reqd_work_group_size(%d, 1, 1))) ", gpuCL.numSegments);
	fStr_appendFormatted(fStr_src, "__kernel void jhp_sieve_primes(__global unsigned int *candidateList, __global unsigned int *vPrimes, __global unsigned int *fixedInverseBase, __global unsigned int* vPrimesTwoInverse, unsigned int vPrimesStart, unsigned int vPrimesToProcess)\r\n");
	fStr_append(fStr_src, "{\r\n");
	fStr_appendFormatted(fStr_src, "__local unsigned int maskC1[%d];\r\n", (gpuCL.sieveMaskSize+3)/4);
	fStr_appendFormatted(fStr_src, "__local unsigned int maskC2[%d];\r\n", (gpuCL.sieveMaskSize+3)/4);
	fStr_appendFormatted(fStr_src, "__local unsigned int maskBT[%d];\r\n", (gpuCL.sieveMaskSize+3)/4);

	fStr_append(fStr_src, "	unsigned int t = get_local_id(0);\r\n");
	fStr_append(fStr_src, "	unsigned int tWorkgroupBase = get_group_id(0);\r\n");
	// multi worker group code:
	fStr_appendFormatted(fStr_src, "	unsigned int sieveDistance = tWorkgroupBase * %d;\r\n", gpuCL.sieveMaskSize*8);
	fStr_appendFormatted(fStr_src, "	unsigned int multiplierBase = sieveDistance;\r\n");
	//fStr_appendFormatted(fStr_src, "	unsigned int primeRangeStart = t * (%d);\r\n", gpuCL.sizeSegment);
	//fStr_appendFormatted(fStr_src, "	unsigned int endRange = startRange + (%d);\r\n", gpuCL.sizeSegment);

	// clear local memory (maybe we can remove this? Not sure if the memory is always initialized as zeroes)
	fStr_appendFormatted(fStr_src, "	for(int i=t*%d; i<(t+1)*%d; i++)\r\n", sieveSize/gpuCL.numSegments/32, sieveSize/gpuCL.numSegments/32);
	fStr_append(fStr_src, " {\r\n");
	fStr_append(fStr_src, " maskC1[i] = 0;\r\n");
	fStr_append(fStr_src, " maskC2[i] = 0;\r\n");
	fStr_append(fStr_src, " maskBT[i] = 0;\r\n");
	fStr_append(fStr_src, " }\r\n");

	fStr_appendFormatted(fStr_src, "	unsigned int primeRangeStart = vPrimesStart + t * (vPrimesToProcess);\r\n");
	fStr_appendFormatted(fStr_src, "	unsigned int primeRangeEnd = primeRangeStart + (vPrimesToProcess);\r\n");
	fStr_append(fStr_src, "	for(int nPrimeSeqX=primeRangeStart; nPrimeSeqX<=primeRangeEnd; nPrimeSeqX++)\r\n");
	fStr_append(fStr_src, "	{\r\n");
	fStr_append(fStr_src, "		unsigned int nPrimeSeq = nPrimeSeqX;\r\n");
	//fStr_append(fStr_src, "		unsigned int nPrimeSeq = nPrimeSeqX/2;\r\n");
	//fStr_append(fStr_src, "		if( nPrimeSeqX&1 )\r\n");
	//fStr_append(fStr_src, "				nPrimeSeq = maxPrimes - nPrimeSeq;\r\n");
	fStr_append(fStr_src, "		unsigned int pU64 = (unsigned int)vPrimes[nPrimeSeq];\r\n");
	fStr_append(fStr_src, "		unsigned int fixedInverseU64 = fixedInverseBase[nPrimeSeq];\r\n");
	// multi worker group code:
	//fStr_append(fStr_src, "		fixedInverseU64 = fixedInverseU64 + ((sieveDistance-fixedInverseU64)+pU64-1)/pU64*pU64;\r\n"); // ((limit-base)+p-1)/p*p
	//fStr_append(fStr_src, "		fixedInverseU64 = fixedInverseU64 % pU64;\r\n"); // ((limit-base)+p-1)/p*p
	
	
	fStr_append(fStr_src, "		unsigned int twoInverseU64 = vPrimesTwoInverse[nPrimeSeq];\r\n");
	//fStr_append(fStr_src, "		// Weave the sieve for the prime\r\n");
	// loop variant:
	//fStr_append(fStr_src, "		for (unsigned int nBiTwinSeq = 0; nBiTwinSeq < nChainLength*2; nBiTwinSeq++)\r\n");
	//fStr_append(fStr_src, "		{\r\n");
	//fStr_append(fStr_src, "			unsigned int nSolvedMultiplier;\r\n");
	//fStr_append(fStr_src, "			if( (nBiTwinSeq&1) == 0 )\r\n");
	//fStr_append(fStr_src, "				nSolvedMultiplier = fixedInverseU64;\r\n");
	//fStr_append(fStr_src, "			else\r\n");
	//fStr_append(fStr_src, "				nSolvedMultiplier = (unsigned int)((((unsigned long)fixedInverseU64) * ((unsigned long)pU64 - (unsigned long)1)) % (unsigned long)pU64);\r\n");
	//fStr_append(fStr_src, "			if( nBiTwinSeq & 1 )\r\n");
	//fStr_append(fStr_src, "				fixedInverseU64 = ((unsigned long)fixedInverseU64*(unsigned long)twoInverseU64)%(unsigned long)pU64;\r\n");
	//fStr_append(fStr_src, "			unsigned int nPrime = vPrimes[nPrimeSeq];\r\n");
	//fStr_append(fStr_src, "			unsigned int rem = (startRange-nSolvedMultiplier) / nPrime;\r\n");
	//fStr_append(fStr_src, "			nSolvedMultiplier = nSolvedMultiplier + rem*nPrime;\r\n");
	//fStr_append(fStr_src, "			if (nBiTwinSeq < nChainLength)\r\n");
	//fStr_append(fStr_src, "				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < endRange; nVariableMultiplier += nPrime)\r\n");
	//fStr_append(fStr_src, "					maskBT[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);\r\n");
	//fStr_append(fStr_src, "			if ( nBiTwinSeq & 1 )\r\n");
	//fStr_append(fStr_src, "				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < endRange; nVariableMultiplier += nPrime)\r\n");
	//fStr_append(fStr_src, "					maskC2[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);\r\n");
	//fStr_append(fStr_src, "			else\r\n");
	//fStr_append(fStr_src, "				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < endRange; nVariableMultiplier += nPrime)\r\n");
	//fStr_append(fStr_src, "					maskC1[nVariableMultiplier>>3] |= 1<<(nVariableMultiplier&7);\r\n");
	//fStr_append(fStr_src, "		}\r\n");

	bool useAtomicOperations = true;
	// unrolled variant:
	uint32 nChainLength = (nBits>>24);
	fStr_append(fStr_src, "			unsigned int nSolvedMultiplier;\r\n");
	fStr_append(fStr_src, "			unsigned int nPrime = vPrimes[nPrimeSeq];\r\n");
	fStr_append(fStr_src, "			unsigned int rem;\r\n");
	for(uint32 i=0; i<nChainLength*2; i++)
	{
		if( i&1 )
		{
			fStr_append(fStr_src, "				nSolvedMultiplier = fixedInverseU64;\r\n");
			fStr_append(fStr_src, "				fixedInverseU64 = ((unsigned long)fixedInverseU64*(unsigned long)twoInverseU64)%(unsigned long)pU64;\r\n");
		}
		else
			//fStr_append(fStr_src, "				nSolvedMultiplier = (unsigned int)((((unsigned long)fixedInverseU64) * ((unsigned long)pU64 - (unsigned long)1)) % (unsigned long)pU64);\r\n");
			fStr_append(fStr_src, "				nSolvedMultiplier = (unsigned int)(-fixedInverseU64 + pU64);\r\n");
	//	//fStr_append(fStr_src, "			rem = (startRange-nSolvedMultiplier) / nPrime;\r\n");
	//	//fStr_append(fStr_src, "			nSolvedMultiplier = nSolvedMultiplier + rem*nPrime;\r\n");
		if (i < nChainLength)
		{
			fStr_append(fStr_src, "				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)\r\n");
			//fStr_append(fStr_src, "					atom_or(&maskC1[nVariableMultiplier>>5], 1<<(nVariableMultiplier&31));\r\n");
			if( useAtomicOperations )
				fStr_append(fStr_src, "					atom_or(&maskBT[nVariableMultiplier>>5], 1<<(nVariableMultiplier&31));\r\n");
			else
				fStr_append(fStr_src, "					maskBT[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);\r\n");
		}
		if( i & 1 )
		{
			fStr_append(fStr_src, "				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)\r\n");
			//fStr_append(fStr_src, "					atom_or(maskC2+(nVariableMultiplier>>5), 1<<(nVariableMultiplier&31));\r\n");
			if( useAtomicOperations )
				fStr_append(fStr_src, "					atom_or(&maskC2[nVariableMultiplier>>5], 1<<(nVariableMultiplier&31));\r\n");
			else
				fStr_append(fStr_src, "					maskC2[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);\r\n");
		}
		else
		{
			fStr_append(fStr_src, "				for (unsigned int nVariableMultiplier = nSolvedMultiplier; nVariableMultiplier < nSieveSize; nVariableMultiplier += nPrime)\r\n");
			if( useAtomicOperations )
				fStr_append(fStr_src, "					atom_or(&maskC1[nVariableMultiplier>>5], 1<<(nVariableMultiplier&31));\r\n");
			else
				fStr_append(fStr_src, "					maskC1[nVariableMultiplier>>5] |= 1<<(nVariableMultiplier&31);\r\n");
			//fStr_append(fStr_src, "					atom_or(maskBT+(nVariableMultiplier>>5), 1<<(nVariableMultiplier&31));\r\n");
		}
	}

	fStr_append(fStr_src, "	}\r\n");

	// output candidates
	fStr_appendFormatted(fStr_src, "const unsigned int numCandidatesPerT = %d;\r\n", gpuCL.candidatesPerT);
	fStr_append(fStr_src, "unsigned int numCandidatesFound = 0;\r\n");
	fStr_appendFormatted(fStr_src, "const unsigned int tStep = (%d / %d);\r\n", sieveSize, gpuCL.numSegments);
	fStr_appendFormatted(fStr_src, "unsigned int canidateListBase = tWorkgroupBase * %d;\r\n", gpuCL.candidatesPerT*gpuCL.numSegments);


	fStr_append(fStr_src, "unsigned int pScanStart = tStep * t;\r\n");
	fStr_append(fStr_src, "unsigned int pScanEnd = tStep * t + tStep;\r\n");
	fStr_append(fStr_src, "for(unsigned int pScan = pScanStart; pScan<pScanEnd; pScan++)\r\n");
	fStr_append(fStr_src, "{\r\n");
	fStr_append(fStr_src, "unsigned int maskVal = 1<<(pScan&31);\r\n");
	fStr_append(fStr_src, "unsigned int maskIdx = pScan>>5;\r\n");
	
	fStr_append(fStr_src, "if( (maskC1[maskIdx] & maskC2[maskIdx] & maskBT[maskIdx] & maskVal) == 0 )\r\n");
	fStr_append(fStr_src, "{\r\n");
	fStr_append(fStr_src, "candidateList[canidateListBase+numCandidatesFound+numCandidatesPerT*t] = multiplierBase+pScan;\r\n");
	fStr_append(fStr_src, "numCandidatesFound++;\r\n");
	fStr_append(fStr_src, "if( numCandidatesFound >= numCandidatesPerT) return;\r\n");
	fStr_append(fStr_src, "}\r\n");
	fStr_append(fStr_src, "}\r\n");
	fStr_append(fStr_src, "candidateList[canidateListBase+numCandidatesFound+numCandidatesPerT*t] = 0xFFFFFFFF;\r\n");

	fStr_append(fStr_src, "}\r\n");

	const char* source = fStr_get(fStr_src);
	size_t src_size = fStr_len(fStr_src);
	cl_program program = clCreateProgramWithSource(openCL_context, 1, &source, &src_size, &clerr);
	if(clerr != CL_SUCCESS)
		printf("Error creating OpenCL program\n");//__debugbreak(); // error handling todo

	// Builds the program
	clerr = clBuildProgram(program, 1, &openCL_device, NULL, NULL, NULL);
	if(clerr != CL_SUCCESS)
		printf("Error compiling OpenCL program\n");//__debugbreak(); // error handling todo
	// Shows the log
	char* build_log;
	size_t log_size;
	// First call to know the proper size
	clGetProgramBuildInfo(program, openCL_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	build_log = (char*)malloc(log_size+1);
	memset(build_log, 0x00, log_size+1);
	// Second call to get the log
	clGetProgramBuildInfo(program, openCL_device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
	build_log[log_size] = '\0';
	puts(build_log);
	free(build_log);

	gpuCL.kernel_sievePrimes = clCreateKernel(program, "jhp_sieve_primes", &clerr);


	



	// 0	unsigned char *maskC1
	// 1	unsigned char *maskC2
	// 2	unsigned char *maskBT
	// 3	unsigned int *vPrimes
	// 4	unsigned int *fixedInverseBase
	// 5	unsigned int* vPrimesTwoInverse
	// 6	unsigned int vPrimesSize



	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 0, sizeof(cl_mem), &gpuCL.buffer_maskC1);
	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 1, sizeof(cl_mem), &gpuCL.buffer_maskC2);
	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 2, sizeof(cl_mem), &gpuCL.buffer_maskBT);
	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 0, sizeof(cl_mem), NULL);
	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 1, sizeof(cl_mem), NULL);
	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 2, sizeof(cl_mem), NULL);


	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 0, sizeof(cl_mem), &gpuCL.mem_candidateList);
	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 1, sizeof(cl_mem), &gpuCL.buffer_primes);
	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 2, sizeof(cl_mem), &gpuCL.buffer_primeBase);
	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 3, sizeof(cl_mem), &gpuCL.buffer_twoInverse);


	fStr_free(fStr_src);
}

void CL_API_CALL pfn_notifyCB (

				  const char *errinfo, 
				  const void *private_info, 
				  size_t cb, 
				  void *user_data
				  )
{
	printf("OpenCL: %s\n", errinfo);
}

void openCL_createContext()
{

	cl_int error = 0;   // Used to handle error codes
	//cl_platform_id platform;

	// Platform
	/*error = clGetPlatformID(&platform);
	if (error != CL_SUCCESS) {
		cout << "Error getting platform id: " << errorMessage(error) << endl;
		exit(error);
	}*/
	cl_platform_id platformIds[32];
	cl_uint numPlatforms = 0;
	clGetPlatformIDs(32, platformIds, &numPlatforms);
	// CL_INVALID_PLATFORM
	printf("OpenCL platforms: %d\n", numPlatforms);
	if( numPlatforms != 1 )
	{
		__debugbreak();
	}
	// Device
	error = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 1, &openCL_device, NULL);
	if (error != CL_SUCCESS) {
		exit(error);
	}
	// Context
	openCL_context = clCreateContext(0, 1, &openCL_device, pfn_notifyCB, NULL, &error);
	if (error != CL_SUCCESS) {
		exit(error);
	}
	// Command-queue
	openCL_queue = clCreateCommandQueue(openCL_context, openCL_device, 0, &error);
	if (error != CL_SUCCESS) {
		exit(error);
	}

	
	
}

int single_modinv (int a, int modulus);

bool ProbablePrimeChainTestFast2(const mpz_class& mpzPrimeChainOrigin, CPrimalityTestParams& testParams, uint32 sieveFlags, uint32 multiplier);

void cSieve_debug(mpz_class& mpzFixedMultiplier, uint32 chainLength, uint32 primeFactors, uint32 sieveSize);

bool BitcoinMinerOpenCL_mineProbableChain(primecoinBlock_t* block, mpz_class& mpzFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, 
									 unsigned int& nTests, unsigned int& nPrimesHit, sint32 threadIndex, mpz_class& mpzHash, unsigned int nPrimorialMultiplier)
{
	// todo2: Once the bug is fixed, reenable the multi-work-group code
	// test

	mpz_class mpzChainOrigin;
	mpz_class mpzFinalFixedMultiplier = mpzFixedMultiplier * mpzHash;
	mpz_class mpzTemp;
	// make sure the kernel is updated
	openCL_generateOrUpdateKernel(block->nBits, minerSettings.nSieveSize);

	// preprocess primes
	uint32 filteredPrimesToSieve = 0;
	uint32 skippedPrimes = 0;
	for(uint32 i=0; i<gpuCL.nCLPrimesToSieve; i++)
	{
		uint32 p = vPrimes[i];
		uint32 fixedFactorMod = mpz_mod_ui(mpzTemp.get_mpz_t(), mpzFinalFixedMultiplier.get_mpz_t(), p);
		if( fixedFactorMod == 0 )
		{
			skippedPrimes++;
			continue;
		}
		//cSieve->primeFactorBase[i].mod = fixedFactorMod; 
		gpuCL.mem_primeBase[filteredPrimesToSieve] = single_modinv(fixedFactorMod, p);
		// todo: The two lines below generate static data -> move to kernel creation
		gpuCL.mem_twoInverse[filteredPrimesToSieve] = single_modinv(2, p);
		gpuCL.mem_primes[filteredPrimesToSieve] = p;
		filteredPrimesToSieve++;
	}

	printf("Launch GPU sieve...\n");

	//memset(gpuCL.mem_maskC1, 0x00, gpuCL.sieveMaskSize);
	//memset(gpuCL.mem_maskC2, 0x00, gpuCL.sieveMaskSize);
	//memset(gpuCL.mem_maskBT, 0x00, gpuCL.sieveMaskSize);

	cl_int clerr;

	//clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.mem_candidateList, true, 0, gpuCL.candidatesPerT*gpuCL.numSegments*sizeof(uint32), gpuCL.buffer_candidateList, 0, NULL, NULL);
	//clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_maskC1, true, 0, gpuCL.sieveMaskSize, gpuCL.mem_maskC1, 0, NULL, NULL);
	//clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_maskC2, true, 0, gpuCL.sieveMaskSize, gpuCL.mem_maskC2, 0, NULL, NULL);
	//clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_maskBT, true, 0, gpuCL.sieveMaskSize, gpuCL.mem_maskBT, 0, NULL, NULL);

	//clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_maskC1, false, 0, gpuCL.sieveMaskSize, NULL, 0, NULL, NULL);
	//clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_maskC2, false, 0, gpuCL.sieveMaskSize, NULL, 0, NULL, NULL);
	//clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_maskBT, false, 0, gpuCL.sieveMaskSize, NULL, 0, NULL, NULL);


	clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_primes, true, 0, sizeof(uint32)*gpuCL.nCLPrimesToSieve, gpuCL.mem_primes, 0, NULL, NULL);
	clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_primeBase, true, 0, sizeof(uint32)*gpuCL.nCLPrimesToSieve, gpuCL.mem_primeBase, 0, NULL, NULL);
	clerr = clEnqueueWriteBuffer(openCL_queue, gpuCL.buffer_twoInverse, true, 0, sizeof(uint32)*gpuCL.nCLPrimesToSieve, gpuCL.mem_twoInverse, 0, NULL, NULL);

	size_t work_dim[1];
	//work_dim[0] = gpuCL.numSegments;
	//size_t work_size[1];
	//work_size[0] = min(32, gpuCL.numSegments);


	work_dim[0] = gpuCL.numSegments * gpuCL.workgroupNum;
	size_t localWorkSize[1];
	localWorkSize[0] = gpuCL.numSegments;//min(1, work_dim[0]);

	cl_event event_kernelExecute;
	uint32 nTime = GetTickCount();
	
	uint32 steps = filteredPrimesToSieve / 1024;
	filteredPrimesToSieve = (filteredPrimesToSieve/gpuCL.numSegments)*gpuCL.numSegments;
	cl_int primeRangeStart = 0;
	cl_int primeRangeWidth = 0;
	
	sint32 sievePasses = 0;
	//while( primeRangeStart < filteredPrimesToSieve )
	//{
	//	primeRangeStart = primeRangeStart + primeRangeWidth;
	//	primeRangeWidth = 32;
	//	if( (primeRangeStart + primeRangeWidth*work_dim[0]) >= filteredPrimesToSieve )
	//		break; // early secure quit

	//	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 6, sizeof(cl_int), &primeRangeStart);
	//	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 7, sizeof(cl_int), &primeRangeWidth);
	//	clerr = clEnqueueNDRangeKernel(openCL_queue, gpuCL.kernel_sievePrimes, 1, NULL, work_dim, work_size, 0, NULL, &event_kernelExecute);
	//	sievePasses++;

	//	primeRangeStart += primeRangeWidth*work_dim[0];

	//}

	primeRangeStart = 0;
	primeRangeWidth = filteredPrimesToSieve / localWorkSize[0];

	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 4, sizeof(cl_int), &primeRangeStart);
	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 5, sizeof(cl_int), &primeRangeWidth);
	//uint32 maxPrimes = primeRangeWidth * work_dim[0];
	//clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 6, sizeof(cl_int), &maxPrimes);
	clerr = clEnqueueNDRangeKernel(openCL_queue, gpuCL.kernel_sievePrimes, 1, NULL, work_dim, localWorkSize, 0, NULL, &event_kernelExecute);
	sievePasses++;
	
	if( GetAsyncKeyState(VK_ESCAPE) )
		exit(0); // remove me

	//sint32 sievePasses = 1;
	//while( primeRangeEnd < filteredPrimesToSieve )
	//{
	//	primeRangeStart = primeRangeEnd;
	//	primeRangeEnd = primeRangeStart + (32+sievePasses*8);
	//	primeRangeEnd = min(primeRangeEnd, filteredPrimesToSieve);
	//	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 6, sizeof(cl_int), &primeRangeStart);
	//	clerr = clSetKernelArg(gpuCL.kernel_sievePrimes, 7, sizeof(cl_int), &primeRangeEnd);
	//	clerr = clEnqueueNDRangeKernel(openCL_queue, gpuCL.kernel_sievePrimes, 1, NULL, work_dim, work_size, 0, NULL, &event_kernelExecute);
	//	sievePasses++;
	//}

	//clFinish();

	clerr = clEnqueueReadBuffer(openCL_queue, gpuCL.mem_candidateList, true, 0, gpuCL.candidatesPerT*gpuCL.numSegments*gpuCL.workgroupNum*sizeof(uint32), gpuCL.buffer_candidateList, 0, NULL, NULL);
	//clerr = clEnqueueReadBuffer(openCL_queue, gpuCL.buffer_maskC1, true, 0, gpuCL.sieveMaskSize, gpuCL.mem_maskC1, 0, NULL, NULL);
	//clerr = clEnqueueReadBuffer(openCL_queue, gpuCL.buffer_maskC2, true, 0, gpuCL.sieveMaskSize, gpuCL.mem_maskC2, 0, NULL, NULL);
	//clerr = clEnqueueReadBuffer(openCL_queue, gpuCL.buffer_maskBT, true, 0, gpuCL.sieveMaskSize, gpuCL.mem_maskBT, 0, NULL, NULL);
	nTime = GetTickCount() - nTime;
	printf("Kernel sieve time and read: %d\n", nTime);
	printf("Sieve size: %d\n", gpuCL.sieveMaskSize*8);
	printf("Sieve passes: %d\n", sievePasses);

	printf("List multipliers: %d\n");
	//for(uint32 t=0; t<gpuCL.numSegments; t++)
	//{
	//	uint32 baseIdx = t*gpuCL.candidatesPerT;
	//	for(uint32 e=0; e<gpuCL.candidatesPerT; e++)
	//	{
	//		uint32 mult = gpuCL.buffer_candidateList[baseIdx+e];
	//		if( mult == 0xFFFFFFFF )
	//			break;
	//		printf("Possible multiplier: 0x%08X -> %d\n", mult, mult);
	//	}
	//}

	//__debugbreak();


	// process chains
	//uint32 nTime = GetTickCount();
	//nTime = GetTickCount()-nTime;
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

	cSieve_debug(mpzFinalFixedMultiplier, (block->nBits>>24), (primeRangeWidth * localWorkSize[0])+skippedPrimes, gpuCL.sieveMaskSize*8*gpuCL.workgroupNum);

	sint32 multIdx = 0;
	for(uint32 t=0; t<gpuCL.numSegments*gpuCL.workgroupNum; t++)
	{
		uint32 baseIdx = t*gpuCL.candidatesPerT;
		for(uint32 e=0; e<gpuCL.candidatesPerT; e++)
		{
			uint32 multiplier = gpuCL.buffer_candidateList[baseIdx+e];
			if( multiplier == 0xFFFFFFFF )
				break;
			if( multiplier == 0 )
				continue;
			printf("gpuSieve mult[%04d]: %d\n", multIdx, multiplier);
			multIdx++;
			//printf("Possible multiplier: 0x%08X -> %d\n", mult, mult);
			mpz_mul_ui(mpzChainOrigin.get_mpz_t(), mpzFinalFixedMultiplier.get_mpz_t(), multiplier);
			nChainLengthCunningham1 = 0;
			nChainLengthCunningham2 = 0;
			nChainLengthBiTwin = 0;

			bool canSubmitAsShare = ProbablePrimeChainTestFast2(mpzChainOrigin, testParams, 0, multiplier);
			nProbableChainLength = max(max(nChainLengthCunningham1, nChainLengthCunningham2), nChainLengthBiTwin);

			if( nProbableChainLength >= 0x01000000 )
			{
				primeStats.numPrimeCandidates++;
			}
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
				nTests = 0;
				primeStats.fourChainCount++;
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
				printf("%s - GPU SHARE FOUND!!! (Th#: %u Multiplier: %d) ---  DIFF: %f    %s    %s\n", 
					sNow, threadIndex, multiplier, (float)((double)nProbableChainLength  / (double)0x1000000), 
					nProbableChainLength >= 0x6000000 ? ">6":"", nProbableChainLength >= 0x7000000 ? ">7":"");
				// submit this share
				if (jhMiner_pushShare_primecoin(blockRawData, block))
					primeStats.foundShareCount ++;
				RtlZeroMemory(blockRawData, 256);
				// dont quit if we find a share, there could be other shares in the remaining prime candidates
				nTests = 0;   // tehere is a good chance to find more shares so search a litle more.
			}
		}
	}

	//uint32 primeCandidates = 0;
	//for(uint32 i=1; i<gpuCL.sieveMaskSize*8; i++)
	//{
	//	uint8 mask = 1<<(i&7);
	//	uint32 maskIdx = i>>3;
	//	if( (gpuCL.mem_maskC1[maskIdx]&mask)!=0 && (gpuCL.mem_maskC2[maskIdx]&mask)!=0 && (gpuCL.mem_maskBT[maskIdx]&mask)!=0 )
	//		continue;
	//	mpzTemp = mpzFinalFixedMultiplier * i;


	//	unsigned int nChainLengthCunningham1 = 0;
	//	unsigned int nChainLengthCunningham2 = 0;
	//	unsigned int nChainLengthBiTwin = 0;
	//	ProbablePrimeChainTest(mpzTemp, block->nBits, true, nChainLengthCunningham1, nChainLengthCunningham2, nChainLengthBiTwin, true);
	//	unsigned int probableChainLength = max(nChainLengthCunningham1, max(nChainLengthCunningham2, nChainLengthBiTwin));
	//	if( probableChainLength >= 0x01000000 )
	//		primeCandidates++;
	//	//printf("%08X\n", probableChainLength);
	//}
	double primalityRatio = (double)primeStats.numPrimeCandidates / (double)primeStats.numTestedCandidates * 100.0;
	printf("Sieve prime ratio: %lf\n", primalityRatio);
	//__debugbreak();

	return false;
}

void BitcoinMiner_openCL(primecoinBlock_t* primecoinBlock, sint32 threadIndex)
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
		BitcoinMinerOpenCL_mineProbableChain(primecoinBlock, mpzFixedMultiplier, fNewBlock, nTriedMultiplier, nProbableChainLength, nTests, nPrimesHit, threadIndex, mpzHash, nPrimorialMultiplier);
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


