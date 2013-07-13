#include"./JHLib.h"


typedef struct
{
	sint32	sizeX;
	sint32	sizeY;
	uint8	bitDepth;
	void*	data;
	//uint8	format;
	//uint32	bytesPerRow;
}bitmap_t;

bitmap_t *bmp_load(char *path);
void bmp_free(bitmap_t *bmp);

BOOL bmp_save(char *path, bitmap_t *bitmap);