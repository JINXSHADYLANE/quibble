#ifndef IMAGE_H
#define IMAGE_H

#include <utils.h>

// Loads images from files
// Supported formats:
// png, jpeg (OS implementation or stb_image)
// dig (fast-loading dgreed format with support for weird pixel formats - 4444, PVRTC)

// dig is png inspired format (lossless, filter+compressor), but much simpler
// and faster to load while achieving comparable compression ratios

// Do not ever change existing values!
typedef enum {
	PF_RGB888 = 0x1,
	PF_RGB565 = 0x2,
	PF_RGBA8888 = 0x3,
	PF_RGBA4444 = 0x4,
	PF_RGBA5551 = 0x5,
	PF_LA88 = 0x6,
	PF_PVRTC2 = 0x10,
	PF_PVRTC4 = 0x20,

	PF_MASK_PVRTC = 0x70,
	PF_MASK_RGBA = 0x0F,
	PF_MASK_PIXEL_FORMAT = 0x7F,
	PF_MASK_PREMUL_ALPHA = 0x80
} PixelFormat;

void* image_load(const char* filename, uint* w, uint* h, PixelFormat* format);

void image_write_tga(const char* filename, uint w, uint h, const Color* pixels);

void image_write_png(const char* filename, uint w, uint h, const Color* pixels);

// Uses lz4
void image_write_dig(const char* filename, uint w, uint h, PixelFormat format, void* pixels);

// Uses no compression
void image_write_dig_quick(const char* filename, uint w, uint h, PixelFormat format, void* pixels);

// Uses deflate
void image_write_dig_hc(const char* filename, uint w, uint h, PixelFormat format, void* pixels);

#endif
