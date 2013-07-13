
typedef struct  
{
	uint32 width;
	uint32 height;
	//uint8 colorDepth;
	uint32 bytesPerRow;
	uint8 *pixelData; /* always format RGBA8888 */
}tgaImage_t;

tgaImage_t* tga_load(char *filePath);
bool tga_save(char *filePath, tgaImage_t* tgaImage);
void tga_free(tgaImage_t *tga);