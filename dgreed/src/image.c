#include "image.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#include "lz4.h"
#include "lz4hc.h"

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

#define RGB565_DECODE(color, r, g, b) \
	(r) = color >> 11; \
	(g) = (color >> 5) & 0x3f; \
	(b) = color & 0x1f

#define RGB565_ENCODE(r, g, b) \
	(((r)&0x1f)<<11)|(((g)&0x3f)<<5)|(((b)&0x1f))

static void _enc_delta16(void* data, uint w, uint h) {
	uint16* img = data;
	for(uint i = 1; i < w * h; ++i) {
		img[i] = img[i] - img[i-1];
	}
}

static void _dec_delta16(void* data, uint w, uint h) {
	uint16* img = data;
	uint16 prev = img[0];
	for(uint i = 1; i < w * h; ++i) {
		img[i] = prev + img[i];
		prev = img[i];
	}
}

static void _enc_delta_rgb565(void* data, uint w, uint h) {
	uint16* img = data;
	uint16 prev = img[0], curr;
	byte pr, pg, pb, cr, cg, cb, dr, dg, db;
	for(uint i = 1; i < w * h; ++i) {
		curr = img[i];
		RGB565_DECODE(curr, cr, cg, cb);
		RGB565_DECODE(prev, pr, pg, pb);

		if(cr >= pr)
			dr = cr - pr;
		else
			dr = (0x20 + (cr - pr)) &  0x1f;

		if(cg >= pg)
			dg = cg - pg;
		else
			dg = (0x40 + (cg - pg)) &  0x3f;

		if(cb >= pb)
			db = cb - pb;
		else
			db = (0x20 + (cb - pb)) &  0x1f;

		prev = curr;
		img[i] = RGB565_ENCODE(dr, dg, db);
	}
}

static void _dec_delta_rgb565(void* data, uint w, uint h) {
	uint16* img = data;
	uint16 prev = img[0], curr;
	byte dr, dg, db, r, g, b;
	for(uint i = 1; i < w * h; ++i) {
		curr = img[i];
		RGB565_DECODE(curr, dr, dg, db);
		RGB565_DECODE(prev, r, g, b);
		curr = RGB565_ENCODE((r+dr), (g+dg), (b+dg));
		img[i] = prev = curr;
	}
}

static int _format_bpp(PixelFormat format) {
	switch(format & PF_MASK_PIXEL_FORMAT) {
		case PF_RGBA8888:
			return 32;
		case PF_RGB888:
			return 24;
		case PF_RGB565:
		case PF_RGBA4444:
		case PF_RGBA5551:
			return 16;
		case PF_PVRTC4:
			return 4;
		case PF_PVRTC2:
			return 2;
		default:
			LOG_ERROR("Unknown dig pixel format");
	}
	return 0;
}

static void* _load_dig(FileHandle f, uint* w, uint* h, PixelFormat* format) {
	assert(sizeof(DIGHeader) == 8);

	DIGHeader hdr;
	file_read(f, &hdr, sizeof(DIGHeader));

	// Check header and fail quickly
	if(hdr.id[0] != 'D' || hdr.id[1] != 'I') {
		file_seek(f, 0);
		return NULL;
	}

	// Swap uint16 bytes on big endian 
#if SOPHIST_endian == SOPHIST_big_endian
	hdr.width = endian_swap2(hdr.width);
	hdr.height = endian_swap2(hdr.height);
#endif

	// Write size
	*w = hdr.width;
	*h = hdr.height;

	// Determine bits per pixel
	uint bpp = _format_bpp(hdr.format);

	// Read the rest of the data
	// (use malloc to behave just like stb_image)
	size_t s = file_size(f) - sizeof(DIGHeader);
	void* pixdata = malloc(s);
	file_read(f, pixdata, s);

	// Decompress if neccessary
	if(hdr.compression & DC_LZ4) {
		size_t decompr_size = (*w * *h * bpp) / 8;	
		void* decompr_data = malloc(decompr_size);
		int processed = LZ4_uncompress(pixdata, decompr_data, decompr_size); 
		free(pixdata);

		if(processed != s) {
			// Compressed data was malformed, back out
			free(decompr_data);
			return NULL;
		}
		else {
			// Everything is ok, move on
			pixdata = decompr_data;
			s = decompr_size;
		}
	}

	// Check if size is right
	if(s != (*w * *h * bpp) / 8) {
		free(pixdata);
		return NULL;
	}

	// Undo delta coding
	if(hdr.compression & DC_DELTA) {
		assert((hdr.format & PF_MASK_PIXEL_FORMAT) == PF_RGB565);
		_dec_delta_rgb565(pixdata, *w, *h);
	}

	return pixdata;
}

void* image_load(const char* filename, uint* w, uint* h, PixelFormat* format) {
	assert(filename && w && h && format);

	void* data = NULL;

	FileHandle f = file_open(filename);

	data = _load_dig(f, w, h, format);
	if(!data) {
		// Load png/jpeg
#ifdef TARGET_IOS
#error Not implemented
#else
		int x, y, comp;
		data = stbi_load_from_file((FILE*)f, &x, &y, &comp, 4); 
		*w = x;
		*h = y;
		*format = PF_RGBA8888;
#endif
	}

	file_close(f);

	return data;
}

void image_write_tga(const char* filename, uint w, uint h, const Color* pixels) {
	// TODO
}

void image_write_dig(const char* filename, uint w, uint h, PixelFormat format, void* pixels) {
	assert(filename && pixels);
	assert(w > 0 && w <= 8192);
	assert(h > 0 && h <= 8192);

	// Try 3 variations:
	// a) Uncompressed
	// b) LZ4
	// c) Delta coding + LZ4 (if pixel format supports delta coding)
	//
	// Choose smaller of b and c if it is at least 1.5 times smaller than a,
	// choose a otherwise.
	
	uint bpp = _format_bpp(format);
	size_t size_uncompr = (w * h * bpp) / 8;

	void* pixels_lz4 = malloc(LZ4_compressBound(size_uncompr));
	size_t size_lz4 = LZ4_compressHC(pixels, pixels_lz4, size_uncompr);

	bool deltacode = (format & PF_MASK_PIXEL_FORMAT) == PF_RGB565;
	if(deltacode) {
		void* pixels_dc = malloc(size_uncompr);
		memcpy(pixels_dc, pixels, size_uncompr);
		_enc_delta_rgb565(pixels_dc, w, h);
		void* pixels_lz4dc = malloc(LZ4_compressBound(size_uncompr));
		size_t size_lz4dc = LZ4_compressHC(pixels_dc, pixels_lz4dc, size_uncompr);

		free(pixels_dc);
		if(size_lz4dc < size_lz4) {
			free(pixels_lz4);
			pixels_lz4 = pixels_lz4dc;
		}
		else {
			free(pixels_lz4dc);
			deltacode = false;
		}
	}

	void* out_pixels;
	size_t out_size;
	uint8 compression = 0;
	if(size_lz4 * 3 < size_uncompr * 2) {
		compression = DC_LZ4 | (deltacode ? DC_DELTA : 0);
		out_pixels = pixels_lz4;
		out_size = size_lz4;
	}
	else {
		out_pixels = pixels;
		out_size = size_uncompr;
	}

	// Fill up header
	DIGHeader hdr = {
		.id = "DI",
		.width = w,
		.height = h,
		.format = format,
		.compression = compression
	};

#if SOPHIST_endian == SOPHIST_big_endian
	hdr.width = endian_swap2(hdr.width);
	hdr.height = endian_swap2(hdr.height);
#endif

	// Write
	FileHandle f = file_create(filename);
	file_write(f, &hdr, sizeof(DIGHeader));
	file_write(f, out_pixels, out_size);
	file_close(f);

	free(pixels_lz4);
}

