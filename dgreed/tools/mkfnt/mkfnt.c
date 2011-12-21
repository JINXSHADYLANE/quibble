#include <stdio.h>
#include <gfx_utils.h>
#include <memory.h>

#include "fnt.h"
#include "binpacking.h"

uint n_glyphs;
int* glyph_codepoints;
byte** glyph_bitmaps;
GlyphMetrics* glyph_metrics;

Pos* glyph_pos;
Color* baked_font;

void mkfnt_init(void) {
	n_glyphs = 'z' - 'a' + 1;

	glyph_codepoints = MEM_ALLOC(sizeof(int) * n_glyphs);
	glyph_bitmaps = MEM_ALLOC(sizeof(byte*) * n_glyphs);
	glyph_metrics = MEM_ALLOC(sizeof(GlyphMetrics) * n_glyphs);

	// Temp
	for(char c = 'a'; c <= 'z'; ++c) 
		glyph_codepoints[c-'a'] = c;
	// End temp
}

void mkfnt_render_glyphs(const char* filename, uint px_size) {
	FntHandle font = fnt_init(filename, px_size);
	
	for(uint i = 0; i < n_glyphs; ++i) {
		glyph_bitmaps[i] = fnt_get_glyph(
				font, glyph_codepoints[i], 
				&glyph_metrics[i]
		);
	}

	fnt_close(font);
}

bool mkfnt_try_arrange(uint size) {
	uint* widths = MEM_ALLOC(sizeof(uint) * n_glyphs);
	uint* heights = MEM_ALLOC(sizeof(uint) * n_glyphs);
	
	for(uint i = 0; i < n_glyphs; ++i) {
		// Leave one pixel padding
		widths[i] = glyph_metrics[i].w + 2;
		heights[i] = glyph_metrics[i].h + 2;
	}

	glyph_pos = bpack(size, size, n_glyphs, widths, heights);

	if(glyph_pos) {
		for(uint i = 0; i < n_glyphs; ++i) {
			glyph_pos[i].x += 1;
			glyph_pos[i].y += 1;
		}
	}
		
	//MEM_FREE(widths);
	//MEM_FREE(heights);

	return glyph_pos != NULL;
}

void blit_8bpp(Color* dest, uint dest_w, uint dest_h, byte* src,
		uint x, uint y, uint w, uint h) {

	for(uint dy = 0; dy < h; ++dy) {
		for(uint dx = 0; dx < w; ++dx) {
			size_t d_idx = IDX_2D(x + dx, y + dy, dest_w);
			size_t s_idx = IDX_2D(dx, dy, w);

			byte c = src[s_idx];
			dest[d_idx] = COLOR_RGBA(c, c, c, 255);
		}
	}
}

void mkfnt_bake(uint size) {
	baked_font = MEM_ALLOC(sizeof(Color) * size * size);

	gfx_fill(baked_font, size, size, COLOR_RGBA(0, 0, 0, 255), 
			0, 0, size, size);

	for(uint i = 0; i < n_glyphs; ++i) {
		blit_8bpp(
				baked_font, size, size, glyph_bitmaps[i], 
				glyph_pos[i].x, glyph_pos[i].y, 
				glyph_metrics[i].w, glyph_metrics[i].h
		);
	}

	gfx_save_tga("font.tga", baked_font, size, size);
}

void mkfnt_close(void) {
	for(uint i = 0; i < n_glyphs; ++i) {
		MEM_FREE(glyph_bitmaps[i]);
	}

	if(glyph_pos)
		MEM_FREE(glyph_pos);
	MEM_FREE(glyph_metrics);
	MEM_FREE(glyph_bitmaps);
	MEM_FREE(glyph_codepoints);
}

int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);
	if(params_count() != 2) {
		printf("Provide font file ant size\n");
		return -1;
	}

	const char* filename = params_get(0);
	uint px_size;
	sscanf(params_get(1), "%d", &px_size);

	mkfnt_init();
	mkfnt_render_glyphs(filename, px_size);

	uint size = 128;
	while(!mkfnt_try_arrange(size))
		size *= 2;

	printf("size = %d\n", size);
	mkfnt_bake(size);

	return 0;
}

