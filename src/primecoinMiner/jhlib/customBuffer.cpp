#include"./JHLib.h"

customBuffer_t* customBuffer_create(sint32 initialLimit, uint32 objectSize)
{
	customBuffer_t* customBuffer = (customBuffer_t*)malloc(sizeof(customBuffer_t));
	RtlZeroMemory(customBuffer, sizeof(customBuffer_t));

	if( initialLimit == 0 ) initialLimit = 4;
	customBuffer->objectLimit = initialLimit;
	customBuffer->objectSize = objectSize;
	customBuffer->objects = (void*)malloc(customBuffer->objectSize * customBuffer->objectLimit);
	customBuffer->isPreallocated = false;

	return customBuffer;
}


// does not automatically free the subitems
void customBuffer_free(customBuffer_t* customBuffer)
{
	free(customBuffer->objects);
	if( customBuffer->isPreallocated == false )
		free(customBuffer);
}

void customBuffer_add(customBuffer_t* customBuffer, void* data)
{
	if( customBuffer->objectCount == customBuffer->objectLimit )
	{
		customBuffer->objectLimit += (customBuffer->objectLimit/2+1);
		customBuffer->objects = (void**)realloc(customBuffer->objects, customBuffer->objectSize * customBuffer->objectLimit);
	}
	RtlCopyMemory(((uint8*)customBuffer->objects)+customBuffer->objectCount*customBuffer->objectSize, data, customBuffer->objectSize);
	customBuffer->objectCount++;
}

void customBuffer_add(customBuffer_t* customBuffer, void* data, uint32 count)
{
	if( customBuffer == NULL )
		return;
	if( customBuffer->objectCount+count >= customBuffer->objectLimit )
	{
		customBuffer->objectLimit += max(customBuffer->objectCount+count, (customBuffer->objectLimit/2+1));
		customBuffer->objects = (void**)realloc(customBuffer->objects, customBuffer->objectSize * customBuffer->objectLimit);
	}
	RtlCopyMemory(((uint8*)customBuffer->objects)+customBuffer->objectCount*customBuffer->objectSize, data, customBuffer->objectSize*count);
	customBuffer->objectCount += count;
}

void customBuffer_insert(customBuffer_t* customBuffer, sint32 insertIndex, void* data)
{
	if( customBuffer->objectCount == customBuffer->objectLimit )
	{
		customBuffer->objectLimit += (customBuffer->objectLimit/2+1);
		customBuffer->objects = (void**)realloc(customBuffer->objects, customBuffer->objectSize * customBuffer->objectLimit);
		// todo: Insert can be optimized when realloc is used
	}
	// shift post-insert part
	uint32 insertByteIndex = insertIndex * customBuffer->objectSize;
	for(sint32 i=(sint32)customBuffer->objectCount; i>=(sint32)insertIndex; i--)
	{
		RtlCopyMemory(((uint8*)customBuffer->objects)+(i+1)*customBuffer->objectSize, ((uint8*)customBuffer->objects)+(i)*customBuffer->objectSize, customBuffer->objectSize);
	}
	RtlCopyMemory(((uint8*)customBuffer->objects)+insertIndex*customBuffer->objectSize, data, customBuffer->objectSize);
	customBuffer->objectCount += 1;
}

uint32 customBuffer_generateHash(customBuffer_t* customBuffer)
{
	uint32 hash = 0xF938B429;
	uint32 size = customBuffer->objectCount * customBuffer->objectSize;
	uint8* p = (uint8*)customBuffer->objects;
	for(uint32 i=0; i<size; i++)
	{
		uint32 c = (uint32)p[i];
		hash ^= c;
		hash = (hash<<3)|(hash>>(32-3));
	}
	return hash;
}

/*
 * Removes the item with the given index
 * Does not maintain order of the elements
 */
void customBuffer_remove(customBuffer_t* customBuffer, uint32 removeIndex)
{
	if( removeIndex >= customBuffer->objectCount )
		return;
	RtlCopyMemory(((uint8*)customBuffer->objects)+removeIndex*customBuffer->objectSize, ((uint8*)customBuffer->objects)+(customBuffer->objectCount-1)*customBuffer->objectSize, customBuffer->objectSize);
	customBuffer->objectCount--;
}

// this function does not check boundaries!
void* customBuffer_get(customBuffer_t* customBuffer, sint32 index)
{
	return (void*)(((uint8*)customBuffer->objects)+index*customBuffer->objectSize);
}

customBuffer_t* customBuffer_duplicate(customBuffer_t* customBufferSource)
{
	customBuffer_t* customBuffer = (customBuffer_t*)malloc(sizeof(customBuffer_t));
	RtlZeroMemory(customBuffer, sizeof(customBuffer_t));

	customBuffer->objectLimit = customBufferSource->objectCount; // limit = count
	customBuffer->objectCount = customBufferSource->objectCount;
	customBuffer->objectSize = customBufferSource->objectSize;
	customBuffer->objects = (void*)malloc(customBuffer->objectSize * customBuffer->objectLimit);
	RtlCopyMemory(customBuffer->objects, customBufferSource->objects, customBuffer->objectSize * customBuffer->objectLimit);
	customBuffer->isPreallocated = false;

	return customBuffer;
}
