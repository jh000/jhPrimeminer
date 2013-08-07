#include"global.h"

#define ALLOC_SPEED_UP_BUFFERS (32*5)
#define ALLOC_SPEED_UP_BUFFER_SIZE (4096) // 4kb

__declspec( thread ) uint8* allocSpeedupBuffer;
__declspec( thread ) uint8* allocSpeedupBufferEnd;
__declspec( thread ) uint32 allocSpeedupMask[ALLOC_SPEED_UP_BUFFERS/32] = {0}; // bit not set -> free, bit set -> used
//__declspec( thread ) uint32 allocSpeedupBufferLog[ALLOC_SPEED_UP_BUFFERS];
//__declspec( thread ) uint32 allocSpeedupBufferLogTime[ALLOC_SPEED_UP_BUFFERS];

uint32 inline _getFirstBitIndex(uint32 mask)
{
	unsigned long index;
	_BitScanForward(&index, mask);
	return index;
}

/*
 * The mallocEx/reallocEx/freeEx code below is used to speed up memory allocation for mpz
 */
void* mallocEx(size_t size)
{
	//printf("malloc\n");
	if( size <= ALLOC_SPEED_UP_BUFFER_SIZE)
	{
		// do a stack walk (debugging)
		//// stack walk
		//uint32* stackOffset = ((uint32*)_AddressOfReturnAddress())-1;
		//uint32 returnAddress = stackOffset[1];
		//uint32 epbBaseAddress = stackOffset[0];
		//uint32 epbBaseAddressOrg = epbBaseAddress;
		//uint32 stackHash = returnAddress;
		//sint32 stackDif = (sint32)((uint32)stackOffset - epbBaseAddress);
		//if( stackDif < 0 ) stackDif = -stackDif;
		//if( stackDif <= 1024*800 )
		//{
		// for(sint32 c=0; c<12; c++)
		// {
		// stackDif = (sint32)((uint32)stackOffset - epbBaseAddress);
		// if( stackDif < 0 ) stackDif = -stackDif;
		// if( stackDif >= 1024*800 )
		// break;
		// stackOffset = (uint32*)(epbBaseAddress);
		// returnAddress = stackOffset[1];
		// epbBaseAddress = stackOffset[0];
		// if( returnAddress == NULL || epbBaseAddress == NULL )
		// break;
		// stackHash += returnAddress;
		// }
		//}
		//// end of stack walk
		for(uint32 i=0; i<(ALLOC_SPEED_UP_BUFFERS/32); i++)
		{
			if( allocSpeedupMask[i] != 0xFFFFFFFF )
			{
				uint32 bufferIndex = _getFirstBitIndex(~allocSpeedupMask[i]);
				if( allocSpeedupMask[i] & (1<<bufferIndex) )
					__debugbreak();
				allocSpeedupMask[i] |= (1<<bufferIndex);
				//allocSpeedupBufferLog[i*32+bufferIndex] = stackHash;
				//allocSpeedupBufferLogTime[i*32+bufferIndex] = GetTickCount();

				return allocSpeedupBuffer + (i*32+bufferIndex)*ALLOC_SPEED_UP_BUFFER_SIZE;
			}
		}
		//printf("All used :( (Stack Hash: %08X)\n", stackHash);
	}
	return malloc(size);
}

void freeEx(void* buffer, size_t oldSize)
{
	//printf("free\n");
	if( buffer >= allocSpeedupBuffer && buffer < allocSpeedupBufferEnd )
	{
		uint32 bufferIndex = ((uint8*)buffer - allocSpeedupBuffer)/ALLOC_SPEED_UP_BUFFER_SIZE;
		allocSpeedupMask[bufferIndex/32] &= ~(1<<(bufferIndex&31));
		return;
	}
	//printf("Free: %08x\n", buffer);
	free(buffer);
}

void* reallocEx(void* buffer, size_t oldSize, size_t newSize)
{
	//printf("realloc\n");
	if( buffer == NULL )
		return mallocEx(newSize);
	if( buffer >= allocSpeedupBuffer && buffer < allocSpeedupBufferEnd )
	{
		if( newSize <= ALLOC_SPEED_UP_BUFFER_SIZE )
			return buffer; // dont need to reallocate anything
		freeEx(buffer, oldSize);
		return mallocEx(newSize);
	}
	return realloc(buffer, newSize);
}

void mallocSpeedupInit()
{
	mp_set_memory_functions(mallocEx, reallocEx, freeEx);
}

void mallocSpeedupInitPerThread()
{
	if( allocSpeedupBuffer )
		return;
	allocSpeedupBuffer = (uint8*)malloc(ALLOC_SPEED_UP_BUFFERS*ALLOC_SPEED_UP_BUFFER_SIZE);
	allocSpeedupBufferEnd = allocSpeedupBuffer + (ALLOC_SPEED_UP_BUFFERS*ALLOC_SPEED_UP_BUFFER_SIZE);
}