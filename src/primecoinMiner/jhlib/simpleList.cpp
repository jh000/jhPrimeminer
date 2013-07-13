#include"./JHLib.h"

// malloc can be a real performance eater - thus introduce 'the simpleList cache' which caches up to 1000 lists and 1000 32-entry buffers

simpleList_t* simpleList_create(sint32 initialLimit)
{
	simpleList_t* simpleList = (simpleList_t*)malloc(sizeof(simpleList_t));
	RtlZeroMemory(simpleList, sizeof(simpleList_t));

	if( initialLimit == 0 ) initialLimit = 4;
	simpleList->objectLimit = initialLimit;
	simpleList->objects = (void**)malloc(sizeof(void*) * simpleList->objectLimit);
	simpleList->isPreallocated = false;
	simpleList->stepScaler = 1;
	return simpleList;
}

void simpleList_create(simpleList_t* simpleList, sint32 initialLimit)
{
	RtlZeroMemory(simpleList, sizeof(simpleList_t));
	if( initialLimit == 0 ) initialLimit = 4;
	simpleList->objectLimit = initialLimit;
	simpleList->objects = (void**)malloc(sizeof(void*) * simpleList->objectLimit);
	simpleList->isPreallocated = true;
	simpleList->stepScaler = 1;
}

void simpleList_create(simpleList_t* simpleList, sint32 initialLimit, void** rawArray)
{
	RtlZeroMemory(simpleList, sizeof(simpleList_t));
	if( initialLimit == 0 ) initialLimit = 4;
	simpleList->objectLimit = initialLimit;
	simpleList->objects = rawArray;
	simpleList->doNotFreeRawData = true;
	simpleList->isPreallocated = true;
	simpleList->stepScaler = 1;
}


// does not automatically free the subitems
void simpleList_free(simpleList_t* simpleList)
{
	if( simpleList->doNotFreeRawData == false )
		free(simpleList->objects);
	if( simpleList->isPreallocated == false )
		free(simpleList);
}

void simpleList_add(simpleList_t* simpleList, void* object) // todo: Via define macro it would be possible to always return a casted object?
{
	if( simpleList->objectCount == simpleList->objectLimit )
	{
		simpleList->objectLimit += (simpleList->objectLimit/2+1)*simpleList->stepScaler;
		if( simpleList->doNotFreeRawData )
		{
			void* oldObjectPtr = simpleList->objects;
			simpleList->objects = (void**)malloc(sizeof(void*) * simpleList->objectLimit);
			RtlCopyMemory(simpleList->objects, oldObjectPtr, sizeof(void*) * simpleList->objectCount);
		}
		else
			simpleList->objects = (void**)realloc(simpleList->objects, sizeof(void*) * simpleList->objectLimit);
		simpleList->doNotFreeRawData = false;
	}
	simpleList->objects[simpleList->objectCount] = object;
	simpleList->objectCount++;
}

// does not add the object if it is already in the list
void simpleList_addUnique(simpleList_t* simpleList, void* object)
{
	for(sint32 i=0; i<simpleList->objectCount; i++)
	{
		if( simpleList->objects[i] == object )
			return;
	}
	// object not found, add item
	if( simpleList->objectCount == simpleList->objectLimit )
	{
		simpleList->objectLimit += (simpleList->objectLimit/2+1)*simpleList->stepScaler;
		if( simpleList->doNotFreeRawData )
		{
			void* oldObjectPtr = simpleList->objects;
			simpleList->objects = (void**)malloc(sizeof(void*) * simpleList->objectLimit);
			RtlCopyMemory(simpleList->objects, oldObjectPtr, sizeof(void*) * simpleList->objectCount);
		}
		else
			simpleList->objects = (void**)realloc(simpleList->objects, sizeof(void*) * simpleList->objectLimit);
		simpleList->doNotFreeRawData = false;
	}
	simpleList->objects[simpleList->objectCount] = object;
	simpleList->objectCount++;
}

// does not add the object if it is already in the list and returns true if it was added
bool simpleList_addUniqueFeedback(simpleList_t* simpleList, void* object)
{
	for(sint32 i=0; i<simpleList->objectCount; i++)
	{
		if( simpleList->objects[i] == object )
			return false;
	}
	// object not found, add item
	if( simpleList->objectCount == simpleList->objectLimit )
	{
		simpleList->objectLimit += (simpleList->objectLimit/2+1)*simpleList->stepScaler;
		if( simpleList->doNotFreeRawData )
		{
			void* oldObjectPtr = simpleList->objects;
			simpleList->objects = (void**)malloc(sizeof(void*) * simpleList->objectLimit);
			RtlCopyMemory(simpleList->objects, oldObjectPtr, sizeof(void*) * simpleList->objectCount);
		}
		else
			simpleList->objects = (void**)realloc(simpleList->objects, sizeof(void*) * simpleList->objectLimit);
		simpleList->doNotFreeRawData = false;
	}
	simpleList->objects[simpleList->objectCount] = object;
	simpleList->objectCount++;
	return true;
}

// Never call _remove while parsing through the list!
bool simpleList_remove(simpleList_t* simpleList, void* object)
{
	for(sint32 i=0; i<simpleList->objectCount; i++)
	{
		if( simpleList->objects[i] == object )
		{
			simpleList->objectCount--;
			simpleList->objects[i] = simpleList->objects[simpleList->objectCount];
			return true;
		}
	}
	return false;
}

// this function does not check boundaries!
void* simpleList_get(simpleList_t* simpleList, sint32 index)
{
	return simpleList->objects[index];
}

/************************************************************************/
/*                         Cached list                                  */
/************************************************************************/

objectCreatorCache_t* simpleListCached_createCache(uint32 listCountPerBlock, uint32 initialEntryCount)
{
	return objectCreatorCache_create(sizeof(simpleListCached_t) + initialEntryCount * sizeof(uint32), listCountPerBlock, 1, 0);
}

simpleListCached_t* simpleListCached_create(objectCreatorCache_t* objectCreatorCache)
{
	simpleListCached_t* simpleListCached = (simpleListCached_t*)objectCreatorCache_getNext(objectCreatorCache);
	
	uint32 objectLimit = (objectCreatorCache->objectSize - sizeof(simpleListCached_t)) / 4;
	//if( initialLimit == 0 ) initialLimit = 4;
	simpleListCached->objectLimit = objectLimit;
	simpleListCached->objects = (void**)(simpleListCached+1);
	simpleListCached->doNotFreeRawData = true;
	simpleListCached->stepScaler = 1;
	return simpleListCached;
}


// does not automatically free the subitems
void simpleListCached_free(simpleListCached_t* simpleListCached)
{
	if( simpleListCached->doNotFreeRawData == false )
		free(simpleListCached->objects);
}

void simpleListCached_add(simpleListCached_t* simpleListCached, void* object) // todo: Via define macro it would be possible to always return a casted object?
{
	if( simpleListCached->objectCount == simpleListCached->objectLimit )
	{
		simpleListCached->objectLimit += (simpleListCached->objectLimit/2+1)*simpleListCached->stepScaler;
		if( simpleListCached->doNotFreeRawData )
		{
			void* oldObjectPtr = simpleListCached->objects;
			simpleListCached->objects = (void**)malloc(sizeof(void*) * simpleListCached->objectLimit);
			RtlCopyMemory(simpleListCached->objects, oldObjectPtr, sizeof(void*) * simpleListCached->objectCount);
		}
		else
			simpleListCached->objects = (void**)realloc(simpleListCached->objects, sizeof(void*) * simpleListCached->objectLimit);
		simpleListCached->doNotFreeRawData = false;
	}
	simpleListCached->objects[simpleListCached->objectCount] = object;
	simpleListCached->objectCount++;
}

// does not add the object if it is already in the list
void simpleListCached_addUnique(simpleListCached_t* simpleListCached, void* object)
{
	for(sint32 i=0; i<simpleListCached->objectCount; i++)
	{
		if( simpleListCached->objects[i] == object )
			return;
	}
	// object not found, add item
	if( simpleListCached->objectCount == simpleListCached->objectLimit )
	{
		simpleListCached->objectLimit += (simpleListCached->objectLimit/2+1)*simpleListCached->stepScaler;
		if( simpleListCached->doNotFreeRawData )
		{
			void* oldObjectPtr = simpleListCached->objects;
			simpleListCached->objects = (void**)malloc(sizeof(void*) * simpleListCached->objectLimit);
			RtlCopyMemory(simpleListCached->objects, oldObjectPtr, sizeof(void*) * simpleListCached->objectCount);
		}
		else
			simpleListCached->objects = (void**)realloc(simpleListCached->objects, sizeof(void*) * simpleListCached->objectLimit);
		simpleListCached->doNotFreeRawData = false;
	}
	simpleListCached->objects[simpleListCached->objectCount] = object;
	simpleListCached->objectCount++;
}

// does not add the object if it is already in the list and returns true if it was added
bool simpleListCached_addUniqueFeedback(simpleListCached_t* simpleListCached, void* object)
{
	for(sint32 i=0; i<simpleListCached->objectCount; i++)
	{
		if( simpleListCached->objects[i] == object )
			return false;
	}
	// object not found, add item
	if( simpleListCached->objectCount == simpleListCached->objectLimit )
	{
		simpleListCached->objectLimit += (simpleListCached->objectLimit/2+1)*simpleListCached->stepScaler;
		if( simpleListCached->doNotFreeRawData )
		{
			void* oldObjectPtr = simpleListCached->objects;
			simpleListCached->objects = (void**)malloc(sizeof(void*) * simpleListCached->objectLimit);
			RtlCopyMemory(simpleListCached->objects, oldObjectPtr, sizeof(void*) * simpleListCached->objectCount);
		}
		else
			simpleListCached->objects = (void**)realloc(simpleListCached->objects, sizeof(void*) * simpleListCached->objectLimit);
		simpleListCached->doNotFreeRawData = false;
	}
	simpleListCached->objects[simpleListCached->objectCount] = object;
	simpleListCached->objectCount++;
	return true;
}

// Never call _remove while parsing through the list!
bool simpleListCached_remove(simpleListCached_t* simpleListCached, void* object)
{
	for(sint32 i=0; i<simpleListCached->objectCount; i++)
	{
		if( simpleListCached->objects[i] == object )
		{
			simpleListCached->objectCount--;
			simpleListCached->objects[i] = simpleListCached->objects[simpleListCached->objectCount];
			return true;
		}
	}
	return false;
}

// this function does not check boundaries!
void* simpleListCached_get(simpleListCached_t* simpleListCached, sint32 index)
{
	return simpleListCached->objects[index];
}

/************************************************************************/
/*                         Cache Creator                                */
/************************************************************************/

/*
 * Allocates a new objectCreatorCache and it's initial set of entries
 * The objectCreatorCache can be used to allocate multiple objects of the same type without calling malloc
 * It can lead to very high speed boosts when a lot of objects (over 1000) have to be allocated
 * The downside is that the memory has to be allocated in advance
 */
objectCreatorCache_t* objectCreatorCache_create(uint32 objectSize, uint32 initialEntryCount, uint32 stepScale, uint32 stepAdd)
{
	if( initialEntryCount == 0 )
		__debugbreak();
	objectCreatorCache_t* objectCreatorCache = (objectCreatorCache_t*)malloc(sizeof(objectCreatorCache_t));
	RtlZeroMemory(objectCreatorCache, sizeof(objectCreatorCache_t));
	objectCreatorCache->currentCacheStep = initialEntryCount;
	objectCreatorCache->currentCacheCount = initialEntryCount;
	objectCreatorCache->currentCacheStepMult = stepScale;
	objectCreatorCache->currentCacheStepAdd = stepAdd;
	objectCreatorCache->objectSize = objectSize;
	objectCreatorCache->dataBlocks = simpleList_create(16);
	objectCreatorCache->currentEmitPointer = (uint8*)malloc(objectCreatorCache->objectSize * objectCreatorCache->currentCacheCount);
	objectCreatorCache->endEmitPointer = objectCreatorCache->currentEmitPointer + (objectCreatorCache->objectSize * objectCreatorCache->currentCacheCount);
	RtlZeroMemory(objectCreatorCache->currentEmitPointer, objectCreatorCache->objectSize * objectCreatorCache->currentCacheCount);
	simpleList_add(objectCreatorCache->dataBlocks, objectCreatorCache->currentEmitPointer);
	return objectCreatorCache;
}

/*
 * Returns the next object from the cache
 * Note that all objects are guaranteed to be initialized to zero
 */
void* objectCreatorCache_getNext(objectCreatorCache_t* objectCreatorCache)
{
	void* objectPtr = objectCreatorCache->currentEmitPointer;
	objectCreatorCache->currentEmitPointer += objectCreatorCache->objectSize;
	if( objectCreatorCache->currentEmitPointer != objectCreatorCache->endEmitPointer )
		return objectPtr;
	// allocate new cache block
	objectCreatorCache->currentCacheCount += objectCreatorCache->currentCacheStep;
	objectCreatorCache->currentCacheStep =  objectCreatorCache->currentCacheStep * objectCreatorCache->currentCacheStepMult + objectCreatorCache->currentCacheStepAdd;
	objectCreatorCache->currentEmitPointer = (uint8*)malloc(objectCreatorCache->objectSize * objectCreatorCache->currentCacheCount);
	objectCreatorCache->endEmitPointer = objectCreatorCache->currentEmitPointer + (objectCreatorCache->objectSize * objectCreatorCache->currentCacheCount);
	RtlZeroMemory(objectCreatorCache->currentEmitPointer, objectCreatorCache->objectSize * objectCreatorCache->currentCacheCount);
	simpleList_add(objectCreatorCache->dataBlocks, objectCreatorCache->currentEmitPointer);
	// return object
	return objectPtr;
}

/*
 * Frees the objectCreatorCache and all reserved entries
 * Any object that was previously returned by the _getNext() method is invalid after this call
 */
void objectCreatorCache_freeAll(objectCreatorCache_t* objectCreatorCache)
{
	for(uint32 i=0; i<objectCreatorCache->dataBlocks->objectCount; i++)
		free(objectCreatorCache->dataBlocks->objects[i]);
	simpleList_free(objectCreatorCache->dataBlocks);
	free(objectCreatorCache);
}