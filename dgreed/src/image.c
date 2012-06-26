#include "image.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#include "lz4.h"

typedef enum {
	DC_NONE = 0,
	DC_LZ4 = 1,
	DC_DELTA = 2
} DIGCompression;

#pragma pack(1)
typedef struct {
	char id[2];
	uint16 width;
	uint16 height;
	uint8 format;	
	uint8 compression;
} DIGHeader;
#pragma pack()

