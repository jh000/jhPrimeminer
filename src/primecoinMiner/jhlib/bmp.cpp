#include"./JHLib.h"



/*
typedef struct tagBITMAPFILEHEADER {
UINT16 bfType; 
UINT32 bfSize; 
UINT16 bfReserved1; 
UINT16 bfReserved2; 
UINT32 bfOffBits; 
} BITMAPFILEHEADER; */


//typedef struct tagBITMAPINFOHEADER { 
//  UINT32 biSize; 
//  UINT32 biWidth; 
//  UINT32 biHeight; 
//  UINT16 biPlanes; 
//  UINT16 biBitCount;
//  UINT32 biCompression; 
//  UINT32 biSizeImage; 
//  UINT32 biXPelsPerMeter; 
//  UINT32 biYPelsPerMeter; 
//  UINT32 biClrUsed; 
//  UINT32 biClrImportant; 
//} BITMAPINFOHEADER; 

bitmap_t *bmp_load(char *path)
{
	HANDLE hBG = CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if( hBG == INVALID_HANDLE_VALUE )
	{
		return 0; 
	}
	else
	{
		BITMAPFILEHEADER bmp_fh;
		BITMAPINFOHEADER bmp_ih;
		DWORD BT;
		ReadFile(hBG, (PVOID)&bmp_fh, sizeof(BITMAPFILEHEADER), &BT, 0);
		ReadFile(hBG, (PVOID)&bmp_ih, sizeof(BITMAPINFOHEADER), &BT, 0);
		// check if format is supported
		if( bmp_ih.biBitCount != 24 )
			return 0;
		// allocate needed memory
		bitmap_t *vbmp = (bitmap_t*)malloc(sizeof(bitmap_t)+(bmp_ih.biWidth*bmp_ih.biHeight*3));
		PUINT8 BitmapData = (PUINT8)(vbmp+1);
		vbmp->sizeX = bmp_ih.biWidth;
		vbmp->sizeY = bmp_ih.biHeight;
		if( vbmp->sizeY < 0 )
			__debugbreak();
		vbmp->bitDepth = 24;
		vbmp->data = BitmapData;

		// load each line directly into memory (BMPs are stored upside-down in most cases, if not, height is negative)
		for(uint32 y=0; y<vbmp->sizeY; y++)
		{
			ReadFile(hBG, (PVOID)(BitmapData+((vbmp->sizeY-1-y)*(vbmp->sizeX))*3), (vbmp->sizeX*3), &BT, 0);
			// read padding
			uint8 padBuffer[4];
			sint32 lineSize = (vbmp->sizeX*3);
			lineSize = 4-(lineSize&3);
			lineSize &= 3;
			if( lineSize )
				ReadFile(hBG, (PVOID)padBuffer, lineSize, &BT, 0);
		}
		CloseHandle(hBG);
		return vbmp;
	}
	CloseHandle(hBG);
	return 0;
}

void bmp_free(bitmap_t *bmp)
{
	free(bmp);
}

BOOL bmp_save(char *path, bitmap_t *bitmap)
{
	HANDLE hBG = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if( hBG == INVALID_HANDLE_VALUE )
	{
		return 0; 
	}
	else
	{
		BITMAPFILEHEADER bmp_fh;
		BITMAPINFOHEADER bmp_ih;
		DWORD BT;

		bmp_fh.bfType      = 0x4d42;
		bmp_fh.bfSize      = 0;
		bmp_fh.bfReserved1 = 0;
		bmp_fh.bfReserved2 = 0;
		bmp_fh.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		bmp_ih.biSize          = sizeof(bmp_ih);
		bmp_ih.biWidth         = bitmap->sizeX;
		bmp_ih.biHeight        = bitmap->sizeY;
		bmp_ih.biPlanes        = 1;
		bmp_ih.biBitCount      = bitmap->bitDepth;
		bmp_ih.biCompression   = 0;
		bmp_ih.biSizeImage     = 0;
		bmp_ih.biXPelsPerMeter = 0;
		bmp_ih.biYPelsPerMeter = 0;
		bmp_ih.biClrUsed       = 0;
		bmp_ih.biClrImportant  = 0;


		sint32 pixelRowSize = (bitmap->sizeX*3);
		pixelRowSize = 4-(pixelRowSize&3);
		pixelRowSize &= 3;

		bmp_ih.biSize = sizeof(BITMAPINFOHEADER);// + sizeof(BITMAPINFOHEADER) + pixelRowSize * bitmap->sizeY;
		bmp_fh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pixelRowSize * bitmap->sizeY;

		WriteFile(hBG, (PVOID)&bmp_fh, sizeof(BITMAPFILEHEADER), &BT, 0);
		WriteFile(hBG, (PVOID)&bmp_ih, sizeof(BITMAPINFOHEADER), &BT, 0);

		PUINT8 BitmapData = (PUINT8)bitmap->data;
		// load each line directly into memory (BMPs are stored upside-down in most cases, if not, height is negative)
		if( bitmap->bitDepth == 24 )
		{
			for(uint32 y=0; y<bitmap->sizeY; y++)
			{
				WriteFile(hBG, (PVOID)(BitmapData+((bitmap->sizeY-1-y)*(bitmap->sizeX))*3), (bitmap->sizeX*3), &BT, 0);
				// padding
				uint8 padBuffer[4] = {0};
				sint32 lineSize = (bitmap->sizeX*3);
				lineSize = 4-(lineSize&3);
				lineSize &= 3;
				if( lineSize )
					WriteFile(hBG, (PVOID)padBuffer, lineSize, &BT, 0);
			}
		}
		else if( bitmap->bitDepth == 32 )
		{
			for(uint32 y=0; y<bitmap->sizeY; y++)
			{
				WriteFile(hBG, (PVOID)(BitmapData+((bitmap->sizeY-1-y)*(bitmap->sizeX))*4), (bitmap->sizeX*4), &BT, 0);
				//TODO: Read align padding data (rows are aligned to 4 bytes?)
			}
		}
		else
			__debugbreak();
		FlushFileBuffers(hBG);
		CloseHandle(hBG);
		return true;
	}
	CloseHandle(hBG);
	return false;
}