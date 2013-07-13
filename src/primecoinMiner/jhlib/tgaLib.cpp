#include"./JHLib.h"

tgaImage_t* tga_load(char *filePath)
{
	stream_t *file = fileMgr_open(filePath);
	if( file == NULL )
		return NULL;
	// get file as stream
	stream_t *in = file;
	// read header
	uint8 hdr_imageIDLength = stream_readU8(in);
	uint8 hdr_paletteType = stream_readU8(in);
	uint8 hdr_imageType = stream_readU8(in);
	//1 = indiziert (Farbpalette) unkomprimiert
	//2 = RGB (24 Bit) unkomprimiert
	//3 = monochrom unkomprimiert
	//9 = indiziert (Farbpalette) lauflängenkodiert
	//10 = RGB (24 Bit) lauflängenkodiert
	//11 = monochrom lauflängenkodiert
	uint16 hdr_paletteStart = stream_readU16(in);
	uint16 hdr_paletteEnd = stream_readU16(in);
	uint16 hdr_paletteColorDepth = stream_readU8(in);
	sint16 hdr_centerX = stream_readS16(in);
	sint16 hdr_centerY = stream_readS16(in);
	uint16 hdr_width = stream_readU16(in);
	uint16 hdr_height = stream_readU16(in);
	uint8 hdr_colorDepth = stream_readU8(in); //gültige Werte sind 1, 8, 15, 16, 24 und 32
	uint8 hdr_pixelAttributes = stream_readU8(in);
	//Bit 4: horizontale Lage des Nullpunkts (0 = links, 1 = rechts)
	//Bit 5: vertikale Lage des Nullpunkts (0 = unten, 1 = oben)
	//Bit 6-7: 0

	// imageID
	uint8 hdr_imageID[255];
	stream_readData(in, hdr_imageID, hdr_imageIDLength);
	uint8 *pixelData = (uint8*)malloc(hdr_width*hdr_height*4);
	uint32 pixelBytesPerRow = hdr_width*4;
	//uint32 pixelDataSize = 0;
	// palette data (optional)
	// todo
	// allocate and setup tga image instance
	tgaImage_t *tgaImage = (tgaImage_t*)malloc(sizeof(tgaImage_t));
	tgaImage->width = hdr_width;
	tgaImage->height = hdr_height;
	tgaImage->pixelData = pixelData;
	tgaImage->bytesPerRow = hdr_width*4;
	// image data
	if( hdr_imageType == 2 )
	{
		// no-palette, uncompressed
		if( hdr_colorDepth == 32 )
		{
			// RGBA
			// read rows in reversed order
			sint32 sizePerRow = hdr_width*4;
			for(sint32 r=0; r<hdr_height; r++)
			{
				stream_readData(in, pixelData+(hdr_height-r-1)*sizePerRow, sizePerRow);
			}
		}
		else
			__debugbreak();
	}
	else
		__debugbreak();
	// developer data
	// not necessary for us, ignore
	stream_destroy(in); // close stream and file
	return tgaImage;
}


bool tga_save(char *filePath, tgaImage_t* tgaImage)
{
	stream_t *file = fileMgr_create(filePath);
	if( file == NULL )
		return false;
	// get file as stream
	stream_t *streamOut = file;
	// read header
	stream_writeU8(streamOut, 0); // hdr_imageIDLength
	stream_writeU8(streamOut, 0); // hdr_paletteType
	stream_writeU8(streamOut, 2); // hdr_imageType
	//2 = RGB (24 Bit) unkomprimiert


	stream_writeU16(streamOut, 0); // hdr_paletteStart
	stream_writeU16(streamOut, 0); // hdr_paletteEnd
	stream_writeU8(streamOut, 0); // hdr_paletteColorDepth
	stream_writeS16(streamOut, 0); // hdr_centerX
	stream_writeS16(streamOut, 0); // hdr_centerY
	stream_writeU16(streamOut, tgaImage->width); // hdr_width
	stream_writeU16(streamOut, tgaImage->height); // hdr_height
	stream_writeU8(streamOut, 32); // hdr_colorDepth
	
	stream_writeU8(streamOut, 0x08); // hdr_pixelAttributes
	////Bit 4: horizontale Lage des Nullpunkts (0 = links, 1 = rechts)
	////Bit 5: vertikale Lage des Nullpunkts (0 = unten, 1 = oben)
	////Bit 6-7: 0

	//// imageID
	//uint8 hdr_imageID[255];
	//stream_readData(in, hdr_imageID, hdr_imageIDLength);
	//uint8 *pixelData = (uint8*)malloc(hdr_width*hdr_height*4);
	//uint32 pixelBytesPerRow = hdr_width*4;
	////uint32 pixelDataSize = 0;
	//// palette data (optional)
	//// todo
	//// allocate and setup tga image instance
	//tgaImage->width = hdr_width;
	//tgaImage->height = hdr_height;
	//tgaImage->pixelData = pixelData;
	//tgaImage->bytesPerRow = hdr_width*4;
	//// image data
	//
	//// RGBA
	stream_writeData(streamOut, tgaImage->pixelData, tgaImage->width*tgaImage->height*4);

	// developer data
	// not necessary for us, ignore
	stream_destroy(streamOut); // close stream and file
	return true;
}

void tga_free(tgaImage_t *tga)
{
	if( tga->pixelData )
		free(tga->pixelData);
	free(tga);
}