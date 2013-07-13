#include"./JHLib.h"

stream_t *stream_create(streamSettings_t *settings, void *object)
{
	stream_t *stream = (stream_t*)malloc(sizeof(stream_t));
	stream->settings = settings;
	stream->object = object;
	stream->bitIndex = 0;
	stream->bitReadIndex = 0;
	stream->bitReadBufferState = 0;
	if( stream->settings->initStream )
		stream->settings->initStream(object, stream);
	return stream;
}

void stream_destroy(stream_t *stream)
{
	if( stream->settings->destroyStream )
		stream->settings->destroyStream(stream->object, stream);
	free(stream);
}

/* writing */
void stream_writeS8(stream_t *stream, char value)
{
	stream->settings->writeData(stream->object, (void*)&value, 1);
}

void stream_writeS16(stream_t *stream, short value)
{
	stream->settings->writeData(stream->object, (void*)&value, 2);
}

void stream_writeS32(stream_t *stream, int value)
{
	stream->settings->writeData(stream->object, (void*)&value, 4);
}

void stream_writeU8(stream_t *stream, uint8 value)
{
	stream->settings->writeData(stream->object, (void*)&value, 1);
}

void stream_writeU16(stream_t *stream, uint16 value)
{
	stream->settings->writeData(stream->object, (void*)&value, 2);
}

void stream_writeU32(stream_t *stream, uint32 value)
{
	stream->settings->writeData(stream->object, (void*)&value, 4);
}

void stream_writeFloat(stream_t *stream, float value)
{
	stream->settings->writeData(stream->object, (void*)&value, 4);
}

uint32 stream_writeData(stream_t *stream, void *data, int len)
{
	return stream->settings->writeData(stream->object, (void*)data, len);
}

void stream_skipData(stream_t *stream, int len)
{
	stream->settings->setSeek(stream->object, len, true);
}


char stream_readS8(stream_t *stream)
{
	char value;
	stream->settings->readData(stream->object, (void*)&value, 1);
	return value;
}

short stream_readS16(stream_t *stream)
{
	short value;
	stream->settings->readData(stream->object, (void*)&value, 2);
	return value;
}

int stream_readS32(stream_t *stream)
{
	int value;
	stream->settings->readData(stream->object, (void*)&value, 4);
	return value;
}

uint8 stream_readU8(stream_t *stream)
{
	uint8 value;
	stream->settings->readData(stream->object, (void*)&value, 1);
	return value;
}

uint16 stream_readU16(stream_t *stream)
{
	uint16 value;
	stream->settings->readData(stream->object, (void*)&value, 2);
	return value;
}

uint32 stream_readU32(stream_t *stream)
{
	uint32 value;
	stream->settings->readData(stream->object, (void*)&value, 4);
	return value;
}

unsigned long long stream_readU64(stream_t *stream)
{
	unsigned long long value;
	stream->settings->readData(stream->object, (void*)&value, 8);
	return value;
}

float stream_readFloat(stream_t *stream)
{
	float value;
	stream->settings->readData(stream->object, (void*)&value, 4);
	return value;
}

uint32 stream_readData(stream_t *stream, void *data, int len)
{
	return stream->settings->readData(stream->object, data, len);
}

//
//char *stream_readLine(stream_t *stream)
//{
//	// todo: optimize this..
//	DWORD currentSeek = SetFilePointer(file->hFile, 0, NULL, FILE_CURRENT);
//	DWORD fileSize = GetFileSize(file->hFile, NULL);
//	DWORD maxLen = fileSize - currentSeek;
//	if( maxLen == 0 )
//		return NULL; // eof reached
//	// begin parsing
//	char *cstr = (char*)memMgr_alloc(NULL, 512);
//	int size = 0;
//	int limit = 512;
//	while( maxLen )
//	{
//		maxLen--;
//		char n = stream_readS8(file);
//		if( n == '\r' )
//			continue; // skip
//		if( n == '\n' )
//			break; // line end
//		cstr[size] = n;
//		size++;
//		if( size == limit )
//			__debugbreak();
//	}
//	cstr[size] = '\0';
//	return cstr;
//}

void stream_setSeek(stream_t *stream, uint32 seek)
{
	stream->settings->setSeek(stream->object, seek, false);
}

uint32 stream_getSeek(stream_t *stream)
{
	return stream->settings->getSeek(stream->object);
}

uint32 stream_getSize(stream_t *stream)
{
	if( stream->settings->getSize == NULL )
		return 0xFFFFFFFF;
	return stream->settings->getSize(stream->object);
}

void stream_setSize(stream_t *stream, uint32 size)
{
	stream->settings->setSize(stream->object, size);
}

void stream_writeBits(stream_t* stream, uint8* bitData, uint32 bitCount)
{
	for(uint32 i=0; i<bitCount; i++)
	{
		uint32 srcByteIndex = i/8;
		uint32 srcBitIndex = i&7;
		uint8 bitValue = (bitData[srcByteIndex]>>srcBitIndex)&1;
		// append bit
		uint32 dstByteIndex = stream->bitIndex/8;
		uint32 dstBitIndex = stream->bitIndex&7;
		stream->bitIndex++;
		if( bitValue )
			stream->bitBuffer[dstByteIndex] |= (1<<dstBitIndex);
		else
			stream->bitBuffer[dstByteIndex] &= ~(1<<dstBitIndex);
		if( stream->bitIndex == 4*8 )
		{
			// write 4 bytes
			stream->settings->writeData(stream->object, stream->bitBuffer, 4);
			stream->bitIndex = 0;
		}
	}
}

void stream_readBits(stream_t* stream, uint8* bitData, uint32 bitCount)
{
	uint32 i=0;
	while( i < bitCount )
	{
		while( stream->bitReadBufferState && i < bitCount )
		{
			// get source bit
			uint32 srcByteIndex = stream->bitReadIndex/8;
			uint32 srcBitIndex = stream->bitReadIndex&7;
			stream->bitReadIndex++;
			stream->bitReadBufferState--;
			uint8 bitValue = (stream->bitReadBuffer[srcByteIndex]>>srcBitIndex)&1;
			// overwrite dst bit
			uint32 dstByteIndex = i/8;
			uint32 dstBitIndex = i&7;
			stream->bitIndex++;
			if( bitValue )
				bitData[dstByteIndex] |= (1<<dstBitIndex);
			else
				bitData[dstByteIndex] &= ~(1<<dstBitIndex);
			i++;
		}
		if( stream->bitReadBufferState == 0 )
		{
			stream->bitReadBuffer[0] = 0;
			stream->settings->readData(stream->object, stream->bitReadBuffer, 1);
			stream->bitReadBufferState += 8;
			stream->bitReadIndex = 0;
		}

		//uint32 srcByteIndex = i/8;
		//uint32 srcBitIndex = i&7;
		//uint8 bitValue = (bitData[srcByteIndex]>>srcBitIndex)&1;
		//// append bit
		//uint32 dstByteIndex = stream->bitIndex/8;
		//uint32 dstBitIndex = stream->bitIndex&7;
		//stream->bitIndex++;
		//if( bitValue )
		//	stream->bitBuffer[dstByteIndex] |= (1<<dstBitIndex);
		//else
		//	stream->bitBuffer[dstByteIndex] &= ~(1<<dstBitIndex);
		//if( stream->bitIndex == 4*8 )
		//{
		//	// write 4 bytes
		//	stream->settings->writeData(stream->object, stream->bitBuffer, 4);
		//	stream->bitIndex = 0;
		//}
	}
}

uint32 stream_copy(stream_t* dest, stream_t* source, uint32 length)
{
	// find ideal copy size
	uint32 copySize = 1024;
	if( length >= (2*1024*1024) )		copySize = 256*1024; // Length over 2MB --> 256KB steps
	else if( length >= (512*1024) )		copySize = 16*1024; // Length over 512KB --> 16KB steps
	else if( length >= (128*1024) )		copySize = 4*1024; // Length over 128KB --> 4KB steps
	// alloc copy buffer
	uint8* copyBuffer = (uint8*)malloc(copySize);
	// start copy loop
	uint32 copyAmount = 0;
	while( length > 0 )
	{
		uint32 stepCopy = min(length, copySize);
		uint32 bytesRead = stream_readData(source, copyBuffer, stepCopy);
		uint32 bytesWritten = stream_writeData(dest, copyBuffer, stepCopy);
		uint32 tBytes = min(bytesRead,bytesWritten);
		if( tBytes == 0 )
			break; // error while copying, exit now
		copyAmount += tBytes;
		length -= stepCopy;
	}
	// free buffer
	free(copyBuffer);
	// return amount of successfully transfered bytes
	return copyAmount;
}


/* memory streams */

//typedef struct  
//{
//	bool readMode;
//
//}streamEx_memoryRange_t;

typedef struct  
{
	//bool readMode;
	uint32 sizeLimit;
	uint8 *buffer;
	uint32 bufferPosition; /* seek */
	uint32 bufferSize;
	uint32 bufferLimit;
	bool disallowWrite;
	bool doNotFreeMemory;
}streamEx_dynamicMemoryRange_t;

//
//uint32 __fastcall streamEx_memoryRange_readData(void *object, void *buffer, uint32 len)
//{
//	return 0;
//}
//
//uint32 __fastcall streamEx_memoryRange_writeData(void *object, void *buffer, uint32 len)
//{
//	return 0;
//}
//
//uint32 __fastcall streamEx_memoryRange_getSize(void *object)
//{
//	return 0;
//}
//
//void __fastcall streamEx_memoryRange_setSize(void *object, uint32 size)
//{
//
//}
//
//uint32 __fastcall streamEx_memoryRange_getSeek(void *object)
//{
//	return 0;
//}
//
//void __fastcall streamEx_memoryRange_setSeek(void *object, sint32 seek, bool relative)
//{
//
//}
//
//void __fastcall streamEx_memoryRange_initStream(void *object, stream_t *stream)
//{
//
//}
//
//void __fastcall streamEx_memoryRange_destroyStream(void *object, stream_t *stream)
//{
//
//}
//
//streamSettings_t streamEx_memoryRange_settings =
//{
//	streamEx_memoryRange_readData,
//	streamEx_memoryRange_writeData,
//	streamEx_memoryRange_getSize,
//	streamEx_memoryRange_setSize,
//	streamEx_memoryRange_getSeek,
//	streamEx_memoryRange_setSeek,
//	streamEx_memoryRange_initStream,
//	streamEx_memoryRange_destroyStream,
//	// general settings
//	true//bool allowCaching;
//};


uint32 __fastcall streamEx_dynamicMemoryRange_readData(void *object, void *buffer, uint32 len)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	uint32 bytesToRead = min(len, memoryRangeObj->bufferSize - memoryRangeObj->bufferPosition);
	RtlCopyMemory(buffer, memoryRangeObj->buffer + memoryRangeObj->bufferPosition, bytesToRead);
	memoryRangeObj->bufferPosition += bytesToRead;
	return bytesToRead;
}

uint32 __fastcall streamEx_dynamicMemoryRange_writeData(void *object, void *buffer, uint32 len)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	if( memoryRangeObj->disallowWrite )
		return 0;
	uint32 nLen = len;
	uint8* bBuffer = (uint8*)buffer;
	uint32 bytesToWrite;
	uint32 overwriteSize = memoryRangeObj->bufferSize - memoryRangeObj->bufferPosition; // amount of bytes that can be written without exceeding the buffer size
	if( overwriteSize )
	{
		bytesToWrite = min(overwriteSize, len);
		RtlCopyMemory(memoryRangeObj->buffer + memoryRangeObj->bufferPosition, bBuffer, bytesToWrite);
		memoryRangeObj->bufferPosition += bytesToWrite;
		nLen -= bytesToWrite;
		bBuffer += bytesToWrite;
	}
	if( nLen == 0 )
		return len;
	// bytes left that exceed the current buffer size
	uint32 bufferBytesLeft = memoryRangeObj->bufferLimit - memoryRangeObj->bufferPosition;
	// need to enlarge buffer?
	if( bufferBytesLeft < nLen )
	{
		uint32 enlargeSize = 0;
		uint32 minimalEnlargeSize = nLen - bufferBytesLeft;
		// calculate ideal enlargement size (important)
		uint32 idealEnlargeSize = memoryRangeObj->bufferLimit/4; // 25% of current buffer size
		if( idealEnlargeSize < minimalEnlargeSize ) // if idealSize < neededSize
			enlargeSize = idealEnlargeSize + minimalEnlargeSize; // take the sum of both
		else
			enlargeSize = idealEnlargeSize; // just use the idealSize
		// check with maximum size
		uint32 maxEnlargeSize = memoryRangeObj->sizeLimit - memoryRangeObj->bufferLimit;
		enlargeSize = min(maxEnlargeSize, enlargeSize);
		if( enlargeSize )
		{
			// enlarge
			uint32 newLimit = memoryRangeObj->bufferLimit + enlargeSize;
			uint8* newBuffer = (uint8*)malloc(newLimit);
			if( newBuffer ) // check if we could allocate the memory
			{
				// copy old buffer and free it
				RtlCopyMemory(newBuffer, memoryRangeObj->buffer, memoryRangeObj->bufferSize);
				free(memoryRangeObj->buffer);
				memoryRangeObj->buffer = newBuffer;
				// set new limit
				memoryRangeObj->bufferLimit = newLimit;
			}
		}
	}
	// write boundary data
	bufferBytesLeft = memoryRangeObj->bufferLimit - memoryRangeObj->bufferPosition;
	bytesToWrite = min(bufferBytesLeft, nLen);
	if( bytesToWrite )
	{
		RtlCopyMemory(memoryRangeObj->buffer + memoryRangeObj->bufferPosition, bBuffer, bytesToWrite);
		memoryRangeObj->bufferPosition += bytesToWrite;
		memoryRangeObj->bufferSize += bytesToWrite;
		nLen -= bytesToWrite;
		bBuffer += bytesToWrite;
	}
	// return amount of written bytes
	return len - nLen;
}

uint32 __fastcall streamEx_dynamicMemoryRange_getSize(void *object)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	return memoryRangeObj->bufferSize;
}

void __fastcall streamEx_dynamicMemoryRange_setSize(void *object, uint32 size)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	if( memoryRangeObj->disallowWrite )
		return;
	if( size < memoryRangeObj->bufferSize )
		memoryRangeObj->bufferSize = size;
}

uint32 __fastcall streamEx_dynamicMemoryRange_getSeek(void *object)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	return memoryRangeObj->bufferPosition;
}

void __fastcall streamEx_dynamicMemoryRange_setSeek(void *object, sint32 seek, bool relative)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	memoryRangeObj->bufferPosition = seek;
	if( memoryRangeObj->bufferPosition < 0 ) memoryRangeObj->bufferPosition = 0;
	if( memoryRangeObj->bufferPosition > memoryRangeObj->bufferSize ) memoryRangeObj->bufferPosition = memoryRangeObj->bufferSize;
}

void __fastcall streamEx_dynamicMemoryRange_initStream(void *object, stream_t *stream)
{
	// all init is already done in streamCreate function
}

void __fastcall streamEx_dynamicMemoryRange_destroyStream(void *object, stream_t *stream)
{
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)object;
	if( memoryRangeObj->doNotFreeMemory == false )
		free(memoryRangeObj->buffer);
	free(object);
}

streamSettings_t streamEx_dynamicMemoryRange_settings =
{
	streamEx_dynamicMemoryRange_readData,
	streamEx_dynamicMemoryRange_writeData,
	streamEx_dynamicMemoryRange_getSize,
	streamEx_dynamicMemoryRange_setSize,
	streamEx_dynamicMemoryRange_getSeek,
	streamEx_dynamicMemoryRange_setSeek,
	streamEx_dynamicMemoryRange_initStream,
	streamEx_dynamicMemoryRange_destroyStream,
	// general settings
	true//bool allowCaching;
};



stream_t* streamEx_fromMemoryRange(void *mem, uint32 memoryLimit)
{
	stream_t *stream = (stream_t*)malloc(sizeof(stream_t));
	RtlZeroMemory(stream, sizeof(stream_t));
	stream->settings = &streamEx_dynamicMemoryRange_settings;
	// init object
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)malloc(sizeof(streamEx_dynamicMemoryRange_t));
	RtlZeroMemory(memoryRangeObj, sizeof(streamEx_dynamicMemoryRange_t));
	stream->object = memoryRangeObj;
	// init object
	memoryRangeObj->bufferSize = memoryLimit;
	memoryRangeObj->buffer = (uint8*)mem;
	memoryRangeObj->bufferPosition = 0;
	memoryRangeObj->disallowWrite = true;
	memoryRangeObj->doNotFreeMemory = true;
	// call subinit
	if( stream->settings->initStream )
		stream->settings->initStream(memoryRangeObj, stream);
	return stream;
}

/*
 * Creates a new dynamically sized memory buffer.
 * @memoryLimit - The maximum size of the dynamic buffer (set to 0xFFFFFFFF to ignore limit)
 * When reading from the buffer you can only read what has already been written. The memory buffer behaves like a virtual file.
 */
stream_t* streamEx_fromDynamicMemoryRange(uint32 memoryLimit)
{
	stream_t *stream = (stream_t*)malloc(sizeof(stream_t));
	RtlZeroMemory(stream, sizeof(stream_t));
	stream->settings = &streamEx_dynamicMemoryRange_settings;
	// init object
	streamEx_dynamicMemoryRange_t* memoryRangeObj = (streamEx_dynamicMemoryRange_t*)malloc(sizeof(streamEx_dynamicMemoryRange_t));
	RtlZeroMemory(memoryRangeObj, sizeof(streamEx_dynamicMemoryRange_t));
	stream->object = memoryRangeObj;
	// alloc 1KB and setup memoryRangeObj
	memoryRangeObj->sizeLimit = memoryLimit;
	if( memoryLimit < 1024 )
		memoryRangeObj->bufferLimit = memoryLimit;
	else
		memoryRangeObj->bufferLimit = 1024;
	memoryRangeObj->bufferSize = 0;
	memoryRangeObj->buffer = (uint8*)malloc(memoryRangeObj->bufferLimit);
	memoryRangeObj->bufferPosition = 0;
	memoryRangeObj->disallowWrite = false;
	// call subinit
	if( stream->settings->initStream )
		stream->settings->initStream(memoryRangeObj, stream);
	return stream;
}


//
//fsdfsdf
//add memory streams
//Modes:
//	- Read from fixed size buffer
//	- write to fixed size buffer (fixed size = Size limit)
//		- write to dynamically growing buffer
//
//		(- Read from callback) The stream api already provides that service by using custom stream objects 
//		(- write to callback) Same..

/*
 * Behavior of memory streams.
 * - They cannot set the seek beyond the current amount of written bytes (setSeek cannot be used to enlarge the buffer)
 * - They can be used to dynamically write and read data (written data can be re-read)
 * - The current memory buffer size can be reset or decreased (setSeek)

 */


/* substreams */

typedef struct  
{
	stream_t* stream;
	sint32 currentOffset;
	// substream settings
	sint32 baseOffset;
	sint32 size;
}streamEx_substream_t;

uint32 __fastcall streamEx_substream_readData(void *object, void *buffer, uint32 len)
{
	streamEx_substream_t* substream = (streamEx_substream_t*)object;
	uint32 readLimit = substream->size - substream->currentOffset;
	if( readLimit < len ) len = readLimit;
	// set seek (in case we share the stream object)
	stream_setSeek(substream->stream, substream->baseOffset + substream->currentOffset);
	uint32 realRead = stream_readData(substream->stream, buffer, len);
	substream->currentOffset += realRead;
	return realRead;
}

uint32 __fastcall streamEx_substream_writeData(void *object, void *buffer, uint32 len)
{	
	__debugbreak(); // no write access for substreams?
	return 0;
}

uint32 __fastcall streamEx_substream_getSize(void *object)
{
	streamEx_substream_t* substream = (streamEx_substream_t*)object;
	return substream->size;
}

void __fastcall streamEx_substream_setSize(void *object, uint32 size)
{
	__debugbreak(); // not implemented 
}

uint32 __fastcall streamEx_substream_getSeek(void *object)
{
	streamEx_substream_t* substream = (streamEx_substream_t*)object;
	return substream->currentOffset;
}

void __fastcall streamEx_substream_setSeek(void *object, sint32 seek, bool relative)
{
	streamEx_substream_t* substream = (streamEx_substream_t*)object;
	substream->currentOffset = seek;
	if( substream->currentOffset < 0 ) substream->currentOffset = 0;
	if( substream->currentOffset > substream->size ) substream->currentOffset = substream->size;
}

void __fastcall streamEx_substream_initStream(void *object, stream_t *stream)
{

}

void __fastcall streamEx_substream_destroyStream(void *object, stream_t *stream)
{
	free(object);
}

streamSettings_t streamEx_substream_settings =
{
	streamEx_substream_readData,
	streamEx_substream_writeData,
	streamEx_substream_getSize,
	streamEx_substream_setSize,
	streamEx_substream_getSeek,
	streamEx_substream_setSeek,
	streamEx_substream_initStream,
	streamEx_substream_destroyStream,
	// general settings
	true//bool allowCaching;
};

/*
 * Creates a read-only substream object.
 * The substream can only access data inside the given startOffset + size range.
 */
stream_t* streamEx_createSubstream(stream_t* mainstream, sint32 startOffset, sint32 size)
{
	stream_t *stream = (stream_t*)malloc(sizeof(stream_t));
	RtlZeroMemory(stream, sizeof(stream_t));
	stream->settings = &streamEx_substream_settings;
	// init object
	streamEx_substream_t* substream = (streamEx_substream_t*)malloc(sizeof(streamEx_dynamicMemoryRange_t));
	RtlZeroMemory(substream, sizeof(streamEx_dynamicMemoryRange_t));
	stream->object = substream;
	// setup substream
	substream->baseOffset = startOffset;
	substream->currentOffset = 0;
	substream->size = size;
	substream->stream = mainstream;
	// done
	return stream;
}

/*
  Other useful stuff
 */

// copies the contents of the stream to a memory buffer
void* streamEx_map(stream_t* stream, sint32* size)
{
	stream_setSeek(stream, 0);
	sint32 rSize = stream_getSize(stream);
	*size = rSize;
	if( rSize == 0 )
		return ""; // return any valid memory address 
	void* mem = malloc(rSize);
	stream_readData(stream, (void*)mem, rSize);
	return mem;
}

sint32 streamEx_readStringNT(stream_t* stream, char* str, uint32 strSize)
{
	for(sint32 i=0; i<strSize-1; i++)
	{
		str[i] = stream_readS8(stream);
		if( str[i] == '\0' )
			return i-1;
	}
	str[strSize-1] = '\0';
	return strSize-1;
}