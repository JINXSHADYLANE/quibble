#include "fnt.h"
#include <memory.h>
#include <utils.h>

// Include stbtt
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u) MEM_ALLOC(x)
#define STBTT_free(x,u) MEM_FREE(x)
#include "stb_truetype.h"

typedef struct {
	stbtt_fontinfo info;
	float scale;
} StbttFont;

FntHandle fnt_init(const char* filename, uint px_size) {
	assert(filename);
	assert(px_size);

	// Read file into a buffer
	FileHandle ttf_file = file_open(filename);
	uint ttf_size = file_size(ttf_file);
	void* ttf_buffer = MEM_ALLOC(ttf_size);
	file_read(ttf_file, ttf_buffer, ttf_size);
	file_close(ttf_file);

	// Init stbtt
	StbttFont* font = MEM_ALLOC(sizeof(StbttFont));
	stbtt_InitFont(&font->info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0)); 
	font->scale = stbtt_ScaleForPixelHeight(&font->info, px_size);

	MEM_FREE(ttf_buffer);

	return (FntHandle)font;
}

void fnt_close(FntHandle fnt) {
	assert(fnt);

	StbttFont* font = (StbttFont*)fnt;
	MEM_FREE(font);
}

FntMetrics fnt_metrics(FntHandle fnt) {
	assert(fnt);

	StbttFont* font = (StbttFont*)fnt;

	int ascent, descent, gap; 
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &gap);

	FntMetrics result = {(float)ascent, (float)descent, (float)gap};
	return result;
}

byte* fnt_get_glyph(FntHandle fnt, int codepoint, GlyphMetrics* metrics) {
	assert(codepoint);
	assert(metrics);
	assert(fnt);

	StbttFont* font = (StbttFont*)fnt;

	int w, h, x_off, y_off;
	byte* bitmap = stbtt_GetCodepointBitmap(&font->info, 0.0f, font->scale, 
			codepoint, &w, &h, &x_off, &y_off);

	metrics->w = w;
	metrics->h = h;
	metrics->x_off = x_off;
	metrics->y_off = y_off;

	int x_bearing, x_advance;
	stbtt_GetCodepointHMetrics(&font->info, codepoint, &x_advance, &x_bearing);

	metrics->x_advance = (float)x_advance * font->scale;
	metrics->x_bearing = (float)x_bearing * font->scale;

	return bitmap;
}
