#include"./JHLib.h"

#ifndef memMgr_alloc
#define memMgr_alloc(x,y) malloc(y)
#define memMgr_free(x,y) free(y)
#endif

extern streamSettings_t fileStreamSettings;

typedef struct  
{
	HANDLE hFile;
	stream_t* stream;
}file_t;

stream_t *fileMgr_open(char *name)
{
	HANDLE hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;
	file_t *file = (file_t*)memMgr_alloc(NULL, sizeof(file_t));
	file->hFile = hFile;
	file->stream = NULL;
	if( file == NULL )
		return NULL;
	if( file->stream == NULL )
	{
		stream_t* stream = stream_create(&fileStreamSettings, file);
		file->stream = stream;
	}
	return file->stream;
}

stream_t *fileMgr_openForAppend(char *name)
{
	HANDLE hFile = CreateFile(name, FILE_GENERIC_READ|FILE_GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;
	file_t *file = (file_t*)memMgr_alloc(NULL, sizeof(file_t));
	file->hFile = hFile;
	file->stream = NULL;
	SetFilePointer(file->hFile, 0, 0, FILE_END);
	if( file->stream == NULL )
	{
		stream_t* stream = stream_create(&fileStreamSettings, file);
		file->stream = stream;
	}
	return file->stream;
}

stream_t *fileMgr_openWriteable(char *name)
{
	HANDLE hFile = CreateFile(name, FILE_GENERIC_READ|FILE_GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;
	file_t *file = (file_t*)memMgr_alloc(NULL, sizeof(file_t));
	file->hFile = hFile;
	stream_t* stream = stream_create(&fileStreamSettings, file);
	file->stream = stream;
	return file->stream;
}

stream_t *fileMgr_create(char *name)
{
	HANDLE hFile = CreateFile(name, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;
	file_t *file = (file_t*)memMgr_alloc(NULL, sizeof(file_t));
	file->hFile = hFile;
	file->stream = NULL;
	if( file->stream == NULL )
	{
		stream_t* stream = stream_create(&fileStreamSettings, file);
		file->stream = stream;
	}
	return file->stream;
}

void fileMgr_close(file_t *file)
{
	if( file )
	{
		if( file->stream )
			free(file->stream); // do not call stream_destroy as it would work recursive
		CloseHandle(file->hFile);
		memMgr_free(NULL, (void*)file);
	}
}


// todo: add cache for writing
void fileMgr_writeS8(file_t *file, char value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(char), &BT, NULL);
}

void fileMgr_writeS16(file_t *file, short value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(short), &BT, NULL);
}

void fileMgr_writeS32(file_t *file, int value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(int), &BT, NULL);
}

void fileMgr_writeU8(file_t *file, unsigned char value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(unsigned char), &BT, NULL);
}

void fileMgr_writeU16(file_t *file, unsigned short value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(unsigned short), &BT, NULL);
}

void fileMgr_writeU32(file_t *file, unsigned int value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(unsigned int), &BT, NULL);
}

void fileMgr_writeFloat(file_t *file, float value)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)&value, sizeof(float), &BT, NULL);
}

void fileMgr_writeData(file_t *file, void *data, int len)
{
	DWORD BT;
	WriteFile(file->hFile, (LPCVOID)data, len, &BT, NULL);
}

void fileMgr_skipData(file_t *file, int len)
{
	SetFilePointer(file->hFile, len, NULL, FILE_CURRENT);
}


char fileMgr_readS8(file_t *file)
{
	DWORD BT;
	char value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(char), &BT, NULL);
	return value;
}

short fileMgr_readS16(file_t *file)
{
	DWORD BT;
	short value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(short), &BT, NULL);
	return value;
}

int fileMgr_readS32(file_t *file)
{
	DWORD BT;
	int value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(int), &BT, NULL);
	return value;
}

unsigned char fileMgr_readU8(file_t *file)
{
	DWORD BT;
	unsigned char value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(unsigned char), &BT, NULL);
	return value;
}

unsigned short fileMgr_readU16(file_t *file)
{
	DWORD BT;
	unsigned short value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(unsigned short), &BT, NULL);
	return value;
}

unsigned int fileMgr_readU32(file_t *file)
{
	DWORD BT;
	unsigned int value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(unsigned int), &BT, NULL);
	return value;
}

unsigned long long fileMgr_readU64(file_t *file)
{
	DWORD BT;
	unsigned long long value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(unsigned long long), &BT, NULL);
	return value;
}

float fileMgr_readFloat(file_t *file)
{
	DWORD BT;
	float value;
	ReadFile(file->hFile, (LPVOID)&value, sizeof(float), &BT, NULL);
	return value;
}

uint32 fileMgr_readData(file_t *file, void *data, int len)
{
	DWORD BT;
	ReadFile(file->hFile, (LPVOID)data, len, &BT, NULL);
	return BT;
}


char *fileMgr_readLine(file_t *file)
{
	// todo: optimize this..
	DWORD currentSeek = SetFilePointer(file->hFile, 0, NULL, FILE_CURRENT);
	DWORD fileSize = GetFileSize(file->hFile, NULL);
	DWORD maxLen = fileSize - currentSeek;
	if( maxLen == 0 )
		return NULL; // eof reached
	// begin parsing
	char *cstr = (char*)memMgr_alloc(NULL, 512);
	int size = 0;
	int limit = 512;
	while( maxLen )
	{
		maxLen--;
		char n = fileMgr_readS8(file);
		if( n == '\r' )
			continue; // skip
		if( n == '\n' )
			break; // line end
		cstr[size] = n;
		size++;
		if( size == limit )
			__debugbreak();
	}
	cstr[size] = '\0';
	return cstr;
}

void fileMgr_setSeek(file_t *file, unsigned int seek)
{
	SetFilePointer(file->hFile, seek, NULL, FILE_BEGIN);
}

unsigned int fileMgr_getSeek(file_t *file)
{
	return SetFilePointer(file->hFile, 0, NULL, FILE_CURRENT);
}

unsigned int fileMgr_getSize(file_t *file)
{
	return GetFileSize(file->hFile, NULL);
}


/* stream helper */

uint32 __fastcall fileMgr_stream_readData(void *object, void *buffer, uint32 len)
{
	file_t *file = (file_t*)object;
	return fileMgr_readData(file, buffer, len);
}

uint32 __fastcall fileMgr_stream_writeData(void *object, void *buffer, uint32 len)
{
	file_t *file = (file_t*)object;
	fileMgr_writeData(file, buffer, len);
	return len;
}

uint32 __fastcall fileMgr_stream_getSize(void *object)
{
	file_t *file = (file_t*)object;
	return GetFileSize(file->hFile, NULL);
}

void __fastcall fileMgr_stream_setSize(void *object, uint32 size)
{
	file_t *file = (file_t*)object;
	DWORD oldFileSeek = SetFilePointer(file->hFile, size, NULL, FILE_CURRENT);
	SetEndOfFile(file->hFile);
	SetFilePointer(file->hFile, oldFileSeek, NULL, FILE_CURRENT);
}


uint32 __fastcall fileMgr_stream_getSeek(void *object)
{
	file_t *file = (file_t*)object;
	return SetFilePointer(file->hFile, 0, NULL, FILE_CURRENT);
}

void __fastcall fileMgr_stream_setSeek(void *object, sint32 seek, bool relative)
{
	file_t *file = (file_t*)object;
	if( relative )
		fileMgr_skipData(file, seek);	
	else
		fileMgr_setSeek(file, seek);
}


void __fastcall fileMgr_stream_destroyStream(void *object, stream_t *stream)
{
	file_t *file = (file_t*)object;
	CloseHandle(file->hFile);
	memMgr_free(NULL, (void*)file);
}

streamSettings_t fileStreamSettings =
{
	fileMgr_stream_readData,//uint32 (__fastcall *readData)(void *object, void *buffer, uint32 len);
	fileMgr_stream_writeData,//uint32 (__fastcall *writeData)(void *object, void *buffer, uint32 len);
	fileMgr_stream_getSize,//uint32 (__fastcall *getSize)(void *object);
	fileMgr_stream_setSize,
	fileMgr_stream_getSeek,//uint32 (__fastcall *getSeek)(void *object);
	fileMgr_stream_setSeek,//void (__fastcall *setSeek)(void *object, sint32 seek, bool relative);
	NULL,//void (__fastcall *initStream)(void *object, stream_t *stream);
	fileMgr_stream_destroyStream,//void (__fastcall *destroyStream)(void *object, stream_t *stream);
	true//bool allowCaching;	
};



///*
// * Creates a unique string for the file. (Outdated: Dont forget to destroy the stream before closing the file)
// * If you close the stream, the file will be also closed (and freed)
// * If you close the file, the stream will also be closed (and freed).
// */
//stream_t *fileMgr_getStreamFromFile(file_t *file)
//{
//	if( file->stream == NULL )
//		file->stream = stream_create(&fileStreamSettings, file);
//	return file->stream;
//}