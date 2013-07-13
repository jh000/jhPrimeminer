typedef struct _packetBuffer_t
{
	uint8 *sendBuffer;
	uint32 cLen;
	uint32 index;
	uint32 limit;
}packetBuffer_t;

void packetBuffer_init(packetBuffer_t *pb, void *mem, uint32 limit);
void packetBuffer_reset(packetBuffer_t *pb);
void packetBuffer_addUINT8(packetBuffer_t *pb, uint8 b);
void packetBuffer_addUINT16(packetBuffer_t *pb, uint16 d);
void packetBuffer_addUINT32(packetBuffer_t *pb, uint32 d);
void packetBuffer_addMemory(packetBuffer_t *pb, unsigned char *d, int len);

void packetBuffer_setUINT16(packetBuffer_t *pb, uint32 offset, uint16 d);
void packetBuffer_setUINT32(packetBuffer_t *pb, uint32 offset, uint32 d);

uint8 packetBuffer_readUINT8(packetBuffer_t *pb);
uint16 packetBuffer_readUINT16(packetBuffer_t *pb);
uint32 packetBuffer_readUINT32(packetBuffer_t *pb);
void packetBuffer_setReadPointer(packetBuffer_t *pb, uint32 offset);

void* packetBuffer_get(packetBuffer_t *pb);
uint32 packetBuffer_length(packetBuffer_t *pb);

#define packetBuffer512_def unsigned char _pbMem[512]; packetBuffer_t pb; packetBuffer_init(&pb, _pbMem, 512)
#define packetBuffer128_def unsigned char _pbMem[128]; packetBuffer_t pb; packetBuffer_init(&pb, _pbMem, 128)

