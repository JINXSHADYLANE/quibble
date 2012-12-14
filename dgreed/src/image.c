#include "image.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#include "lz4.h"
#include "lz4hc.h"

#include "darray.h"
#include "datastruct.h"

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

typedef enum {
	DC_NONE = 0,
	DC_LZ4 = 1,
	DC_DELTA = 2,
	DC_DEFLATE = 4,
    DC_PALETTIZE = 8
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

#pragma pack(1)
typedef struct {
	byte  idlength;
	byte  colourmaptype;
	byte  datatypecode;
	short int colourmaporigin;
	short int colourmaplength;
	byte  colourmapdepth;
	short int x_origin;
	short int y_origin;
	int16 width;
	int16 height;
	byte  bitsperpixel;
	byte  imagedescriptor;
} TGAHeader;
#pragma pack()

#define RGB565_DECODE(color, r, g, b) \
	(r) = color >> 11; \
	(g) = (color >> 5) & 0x3f; \
	(b) = color & 0x1f

#define RGB565_ENCODE(r, g, b) \
	(((r)&0x1f)<<11)|(((g)&0x3f)<<5)|(((b)&0x1f))

#if 0

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

#endif

static void _dec_delta_rgb565(void* data, uint w, uint h) {
	uint16* img = data;
	uint16 prev = img[0], curr;
	byte dr, dg, db, r, g, b;
	for(uint i = 1; i < w * h; ++i) {
		curr = img[i];
		RGB565_DECODE(curr, dr, dg, db);
		RGB565_DECODE(prev, r, g, b);
		curr = RGB565_ENCODE((r+dr), (g+dg), (b+db));
		img[i] = prev = curr;
	}
}

typedef struct {
    uint16 colour;
    uint count;
} ColourDef;

static int _comp_colour_defs( const void* a, const void* b ) {
    const ColourDef* const ca = a;
    const ColourDef* const cb = b;
    return (int)cb->count - (int)ca->count;
}

static void* _palettize_rgb565(uint16* in, uint insize, uint* outsize) {
    assert(in && insize && outsize);
    DArray out = darray_create(1, 0);

    // Calculate histogram of all colours
    uint histogram[MAX_UINT16+1] = {0};
    uint unique_colours = 0;
    for(uint i = 0; i < insize; ++i) {
        histogram[in[i]]++; 
        if(histogram[in[i]] == 1)
            unique_colours++;
    }

    // Get pairs of all unique colours and their use count, sort it
    assert(unique_colours > 0);
    ColourDef* cdefs = malloc(unique_colours * sizeof(ColourDef));
    for(uint i = 0, idx = 0; i <= MAX_UINT16; ++i) {
        if(histogram[i] > 0) {
            ColourDef def = {
                .colour = (uint16)i,
                .count = histogram[i]
            };
            cdefs[idx++] = def;
        }
    }
    qsort(cdefs, unique_colours, sizeof(ColourDef), _comp_colour_defs);

    // Assign 255 most used colours codes in range [0, 254],
    // build palette
    uint* codes = histogram;
    memset(codes, 0xFF, sizeof(histogram));
    uint covered_pixels = 0;
    for(uint i = 0; i < 255; ++i) {
        histogram[cdefs[i].colour] = i;
        darray_append_multi(&out, &cdefs[i].colour, 2);
        covered_pixels += cdefs[i].count;
    }
    free(cdefs);

    // Encode image using new palette, prefix colours not in
    // palette with 0xFF
    for(uint i = 0; i < insize; ++i) {
        byte code = codes[in[i]];
        darray_append(&out, &code);
        if(code == 0xFF) {
            darray_append_multi(&out, &in[i], 2);
        }
    }

    void* outdata = malloc(out.size);
    *outsize = out.size;
    memcpy(outdata, out.data, out.size);
    darray_free(&out);
    return outdata;
}

static void* _unpalettize_rgb565(void* in, uint insize, uint w, uint h) {
	// Next 255 uint16s is palette, next is palettized image data
	uint16* palette = in;
	byte* data = in + 255 * 2;

	uint16* out = malloc(w * h * 2);
	uint idx = 0;

	// Decode
	for(uint i = 0; i + 255 * 2 < insize; ++i) {
		if(data[i] < 0xFF) {
			out[idx++] = palette[data[i]];
		}
		else {
			out[idx++] = *((uint16*)&data[i+1]);
			i += 2;
		}
	}
	assert(idx == w*h);

	return out;
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
		case PF_LA88:
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
    *format = hdr.format;

	// Determine bits per pixel
	uint bpp = _format_bpp(hdr.format);

	// Read the rest of the data
	// (use malloc to behave just like stb_image)
	size_t s = file_size(f) - sizeof(DIGHeader);
	void* pixdata = malloc(s);
	file_read(f, pixdata, s);

	// Decompress if neccessary
	size_t decompr_size = (*w * *h * bpp) / 8;
	if((hdr.compression & DC_LZ4) || (hdr.compression & DC_DEFLATE)) {
		if(hdr.compression & DC_PALETTIZE) {
			// Override decompressed size if palettized
			decompr_size = *((uint*)pixdata);
			pixdata += 4;
			s -= 4;
		}
		void* decompr_data = malloc(decompr_size);

		size_t processed = decompr_size;
		if(hdr.compression & DC_LZ4)
			processed = LZ4_uncompress(pixdata, decompr_data, decompr_size);
		else {
			if(mz_uncompress(decompr_data, &processed, pixdata, s) != MZ_OK)
                LOG_ERROR("miniz uncompress error");
            processed = s;
        }

        if(hdr.compression & DC_PALETTIZE)
            pixdata -= 4;
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

	// Undo palettization
	if(hdr.compression & DC_PALETTIZE) {
		assert((hdr.format & PF_MASK_PIXEL_FORMAT) == PF_RGB565);
		void* rawpix = _unpalettize_rgb565(pixdata, decompr_size, *w, *h);
		free(pixdata);
		pixdata = rawpix;
        s = (*w * *h * 2);
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

#ifdef TARGET_IOS
extern void* ios_load_image(const char* filename, uint* w, uint* h, PixelFormat* format);
#endif

void* image_load(const char* filename, uint* w, uint* h, PixelFormat* format) {
	assert(filename && w && h && format);

	void* data = NULL;

	FileHandle f = file_open(filename);

	data = _load_dig(f, w, h, format);
	if(!data) {
		// Load png/jpeg
#ifdef TARGET_IOS
		file_close(f);
		data = ios_load_image(filename, w, h, format);
#else
		int x, y, comp;
		data = stbi_load_from_file((FILE*)f, &x, &y, &comp, 4);
		file_close(f);
		*w = x;
		*h = y;
		*format = PF_RGBA8888;
#endif
	}

	return data;
}

void image_write_tga(const char* filename, uint w, uint h, const Color* pixels) {
	// Fill up header
	TGAHeader hdr = {
        .idlength = 0,
        .colourmaptype = 0,
        .datatypecode = 2,
        .colourmaporigin = 0,
        .colourmaplength = 0,
        .colourmapdepth = 0,
        .x_origin = 0,
        .y_origin = 0,
        .width = w,
        .height = h,
        .bitsperpixel = 32,
        .imagedescriptor = 8
	};

	Color * pix = (Color *) malloc(w * h * sizeof(Color));
	byte a, r, g, b;

	for(int i = 0; i < w * h; i++) {
    	pix[i] = pixels[i];
       	// Swap r byte with b byte (argb -> abgr)
       	a = (pix[i] >> 24);
       	r = (pix[i] >> 16);
       	g = (pix[i] >> 8);
        b = (pix[i] >> 0);
        pix[i] = ((a << 24) | (b << 16) | (g << 8)  | (r << 0));
    }

    size_t out_size = (w * h * 32) / 8;

    // Write
    FileHandle f = file_create(filename);
	file_write(f, &hdr, sizeof(TGAHeader));
 	file_write(f, pix, out_size);
    file_close(f);

    free(pix);
}

void image_write_png(const char* filename, uint w, uint h, const Color* pixels) {
	size_t out_size;

	void *pix = tdefl_write_image_to_png_file_in_memory(pixels, w, h, 4, &out_size);

	// Write
	FileHandle f = file_create(filename);
	file_write(f, pix, out_size);
	file_close(f);

	free(pix);
}

void image_write_dig(const char* filename, uint w, uint h, PixelFormat format, void* pixels) {
	assert(filename && pixels);
	assert(w > 0 && w <= 8192);
	assert(h > 0 && h <= 8192);

	// Simply try to compress pixels with lz4,
	// or output raw if savings are less than 1k

	uint bpp = _format_bpp(format);
	size_t size_uncompr = (w * h * bpp) / 8;

	void* pixels_lz4 = malloc(LZ4_compressBound(size_uncompr));
	size_t size_lz4 = LZ4_compressHC(pixels, pixels_lz4, size_uncompr);

	void* out_pixels;
	size_t out_size;
	uint8 compression = 0;

	if(size_lz4 + 1024 < size_uncompr) {
		compression = DC_LZ4;
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

void image_write_dig_quick(const char* filename, uint w, uint h, PixelFormat format, void* pixels) {
	assert(filename && pixels);
	assert(w > 0 && w <= 8192);
	assert(h > 0 && h <= 8192);

	// Output uncompressed dig
        
	uint bpp = _format_bpp(format);
	size_t size_uncompr = (w * h * bpp) / 8;

	void* out_pixels;
	size_t out_size;
	uint8 compression = 0;
	
    out_pixels = pixels;
    out_size = size_uncompr;

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
}

void image_write_dig_hc(const char* filename, uint w, uint h, PixelFormat format, void* pixels) {
	assert(filename && pixels);
	assert(w > 0 && w <= 8192);
	assert(h > 0 && h <= 8192);

	// Try 3 variations:
	// a) Uncompressed
	// a) Deflate
	// b) Palettize + Deflate 
	//
	// Choose the smaller.

	uint bpp = _format_bpp(format);
	size_t size_uncompr = (w * h * bpp) / 8;

	void* pixels_defl = malloc(size_uncompr);
	size_t size_defl = size_uncompr;
    int err = mz_compress2(pixels_defl, &size_defl, pixels, size_uncompr, 9);
	if(err != MZ_OK) {
        printf("miniz error %d\n", err);
        LOG_ERROR("miniz error %d\n", err);
    }

	bool palettize = (format & PF_MASK_PIXEL_FORMAT) == PF_RGB565;
    uint pixels_dc_size = 0;
	if(palettize) {
		void* pixels_dc = _palettize_rgb565(pixels, w*h, &pixels_dc_size);
		void* pixels_defldc = malloc(pixels_dc_size);
		size_t size_defldc = pixels_dc_size;
		err = mz_compress2(pixels_defldc, &size_defldc, pixels_dc, pixels_dc_size, 9);
        if(err != MZ_OK) {
            printf("miniz error %d\n", err);
            LOG_ERROR("miniz error %d\n", err);
        }

		free(pixels_dc);
		if(size_defldc < size_defl) {
			free(pixels_defl);
			pixels_defl = pixels_defldc;
            size_defl = size_defldc;
		}
		else {
			free(pixels_defldc);
			palettize = false;
		}
	}

	void* out_pixels;
	size_t out_size;
	uint8 compression = 0;
	if(size_defl < size_uncompr) {
		compression = DC_DEFLATE | (palettize ? DC_PALETTIZE : 0);
		out_pixels = pixels_defl;
		out_size = size_defl;
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
	if(palettize) {
		assert(pixels_dc_size);
		file_write_uint32(f, pixels_dc_size);
	}
	file_write(f, out_pixels, out_size);
	file_close(f);

	free(pixels_defl);
}

