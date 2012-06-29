#include <image.h>
#include <stdio.h>
#include <memory.h>

#define STBI_HEADER_FILE_ONLY
#include <stb_image.c>

static void* _to_rgb888(Color* data, uint w, uint h) {
	byte* out = malloc(w * h * 3);
	byte r, g, b, a;
	for(uint i = 0, j = 0; i < w * h; ++i, j += 3) {
		COLOR_DECONSTRUCT(data[i], r, g, b, a);
		out[j+0] = r;
		out[j+1] = g;
		out[j+2] = b;
	}
	return out;
}

#define RGB565_ENCODE(r, g, b) \
	(((r)&0x1f)<<11)|(((g)&0x3f)<<5)|(((b)&0x1f))

static void* _to_rgb565(Color* data, uint w, uint h) {
	uint16* out = malloc(w * h * 2);
	byte r, g, b, a;
	for(uint i = 0; i < w * h; ++i) {
		COLOR_DECONSTRUCT(data[i], r, g, b, a);
		r >>= 3;
		g >>= 2;
		b >>= 3;
		out[i] = RGB565_ENCODE(r, g, b);
	}
	return out;
}

#define RGBA4444_ENCODE(r, g, b, a) \
	(((a)&0xF)|(((b)&0xF)<<4)|(((g)&0xF)<<8)|(((r)&0xF)<<12))

static void* _to_rgba4444(Color* data, uint w, uint h) {
	uint16* out = malloc(w * h * 2);
	byte r, g, b, a;
	for(uint i = 0; i < w * h; ++i) {
		COLOR_DECONSTRUCT(data[i], r, g, b, a);
		r >>= 4;
		g >>= 4;
		b >>= 4;
		a >>= 4;
		out[i] = RGBA4444_ENCODE(r, g, b, a);
	}
	return out;
}

#define RGBA5551_ENCODE(r, g, b, a) \
	(((a)&0x1)|(((g)&0x1F)<<1)|(((b)&0x1F)<<6)|(((a)&0x1F)<<11))

static void* _to_rgba5551(Color* data, uint w, uint h) {
	uint16* out = malloc(w * h * 2);
	byte r, g, b, a;
	for(uint i = 0; i < w * h; ++i) {
		COLOR_DECONSTRUCT(data[i], r, g, b, a);
		r >>= 3;
		g >>= 3;
		b >>= 3;
		a >>= 7;
		out[i] = RGBA5551_ENCODE(r, g, b, a);
	}
	return out;
}

static void* _format(Color* data, uint w, uint h, PixelFormat format) {
	void* out = NULL;
	switch(format) {
		case PF_RGB888:
			out = _to_rgb888(data, w, h);
			break;
		case PF_RGB565:
			out = _to_rgb565(data, w, h);
			break;
		case PF_RGBA8888:
			out = MEM_ALLOC(w * h * 4);
			memcpy(out, data, w * h * 4);
			break;
		case PF_RGBA4444:
			out = _to_rgba4444(data, w, h);
			break;
		case PF_RGBA5551:
			out = _to_rgba5551(data, w, h);
			break;
		default:
			printf("format is not yet supported\n");
			LOG_ERROR("unsupported format");
	}
	return out;
}

static void _premul_alpha(Color* data, uint w, uint h) {
	byte r, g, b, a;
	for(uint i = 0; i < w * h; ++i) {
		COLOR_DECONSTRUCT(data[i], r, g, b, a);
		r = (r * a) >> 8;
		g = (g * a) >> 8;
		b = (b * a) >> 8;
		data[i] = COLOR_RGBA(r, g, b, a);
	}
}

static PixelFormat _str_to_format(const char* str) {
	const struct {
		const char* str;
		PixelFormat format;
	} formats[] = {
		{"RGB888", PF_RGB888},
		{"RGB565", PF_RGB565},
		{"RGBA8888", PF_RGBA8888},
		{"RGBA4444", PF_RGBA4444},
		{"RGBA5551", PF_RGBA5551},
		{"PVRTC2", PF_PVRTC2},
		{"PVRTC4", PF_PVRTC4},
	};

	for(uint i = 0; i < sizeof(formats)/sizeof(formats[0]); ++i) {
		if(strcmp(formats[i].str, str) == 0)
			return formats[i].format;
	}
	printf("Unknown format %s", str);
	LOG_ERROR("Unknown format");
	return 0;
}

int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);

	uint n_params = params_count();

	if(n_params == 0) {
		printf("mkdig - makes dig files out of ordinary images\n");
		printf("usage: mkdig [options] file\n");
		printf("  -v\tverbose mode\n");
		printf("  -p\tpremultiply alpha\n");
		printf("  -f\toutput pixel format (default RGBA4444)\n");
		printf("  -o\toutput filename (default out.dig)\n\n");
		printf("Pixel formats:\n");
		printf("	RGB888, RGB565, RGBA8888,\n");
		printf("	RGBA4444, RGBA5551, PVRTC2, PVRTC4\n");
		printf("\n");
		return -1;
	}

	bool premul = false;
	bool verbose = false;
	PixelFormat format = PF_RGBA4444;
	const char* in = NULL;
	const char* out = "out.dig";

	for(uint i = 0; i < n_params; ++i) {
		if(strcmp(params_get(i), "-v") == 0)
			verbose = 0;
		else if(strcmp(params_get(i), "-p") == 0)
			premul = true;
		else if(strcmp(params_get(i), "-f") == 0)
			format = _str_to_format(params_get(++i));
		else if(strcmp(params_get(i), "-o") == 0) 
			out = params_get(++i);
		else if(!in)
			in = params_get(i);
	}

	if(!in) {
		printf("provide input file name!\n");
		return -1;
	}

	int w, h, comp;
	void* data = stbi_load(in, &w, &h, &comp, 4);
	if(!data) {
		printf("can't read %s\n", in);
	}

	if(premul)
		_premul_alpha(data, w, h);

	void* pixels = _format(data, w, h, format);

	if(premul)
		format |= PF_MASK_PREMUL_ALPHA;

	image_write_dig(out, w, h, format, pixels);

	free(pixels);
	free(data);

	return 0;
}

