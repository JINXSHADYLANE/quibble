#include <stdio.h>
#include <gfx_utils.h>
#include <memory.h>

#include "fnt.h"
#include "binpacking.h"

uint n_glyphs;
int* glyph_codepoints;
byte** glyph_bitmaps;
GlyphMetrics* glyph_metrics;
FntMetrics font_metrics;

Pos* glyph_pos;
Color* baked_font;

const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,\"\'/|\\!?+-=*()[]{}@#$%&_ ";

void mkfnt_init(void) {
	n_glyphs = strlen(chars);

	glyph_codepoints = MEM_ALLOC(sizeof(int) * n_glyphs);
	glyph_bitmaps = MEM_ALLOC(sizeof(byte*) * n_glyphs);
	glyph_metrics = MEM_ALLOC(sizeof(GlyphMetrics) * n_glyphs);

	for(uint i = 0; i < n_glyphs; ++i)
		glyph_codepoints[i] = chars[i];
}

void mkfnt_render_glyphs(const char* filename, uint px_size) {
	FntHandle font = fnt_init(filename, px_size);

	font_metrics = fnt_metrics(font);
	
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
		
	MEM_FREE(widths);
	MEM_FREE(heights);

	return glyph_pos != NULL;
}

void blit_8bpp(Color* dest, uint dest_w, uint dest_h, byte* src,
		uint x, uint y, uint w, uint h) {

	for(uint dy = 0; dy < h; ++dy) {
		for(uint dx = 0; dx < w; ++dx) {
			size_t d_idx = IDX_2D(x + dx, y + dy, dest_w);
			size_t s_idx = IDX_2D(dx, dy, w);

			byte c = src[s_idx];
			dest[d_idx] = COLOR_RGBA(255, 255, 255, c);
		}
	}
}

void mkfnt_bake(uint size, const char* tex_path) {
	baked_font = MEM_ALLOC(sizeof(Color) * size * size);

	gfx_fill(baked_font, size, size, COLOR_RGBA(255, 255, 255, 0), 
			0, 0, size, size);

	for(uint i = 0; i < n_glyphs; ++i) {
		blit_8bpp(
				baked_font, size, size, glyph_bitmaps[i], 
				glyph_pos[i].x, glyph_pos[i].y, 
				glyph_metrics[i].w, glyph_metrics[i].h
		);
	}

	gfx_save_tga(tex_path, baked_font, size, size);
}

#pragma pack(1)
typedef struct {
	signed short id, x, y, width, height, xoffset, yoffset, xadvance;
} GlyphDesc;	
#pragma pack()

void mkfnt_write_bft(const char* tex_path) {
	char* bft_path = path_change_ext(tex_path, "bft");	

	FileHandle f = file_create(bft_path);
	MEM_FREE(bft_path);

	// Magic file id
	file_write_uint32(f, FOURCC('F', 'O', 'N', 'T'));

	// Font height
	file_write_uint16(f, abs((font_metrics.ascent - font_metrics.descent)/100.0f));

	// Atlas texture filename
	file_write_uint16(f, strlen(tex_path));
	printf("%d %s\n", strlen(tex_path), tex_path);
	file_write(f, tex_path, strlen(tex_path));

	// Number of glyphs 
	file_write_uint16(f, n_glyphs);

	// Write glyphs one by one
	for(uint i = 0; i < n_glyphs; ++i) {
		GlyphDesc glyph_desc = { 
			.id = glyph_codepoints[i],
			.x = glyph_pos[i].x,
			.y = glyph_pos[i].y,
			.width = glyph_metrics[i].w,
			.height = glyph_metrics[i].h,
			.xoffset = glyph_metrics[i].x_off,
			.yoffset = glyph_metrics[i].y_off,
			.xadvance = glyph_metrics[i].x_advance
		};
	
		glyph_desc.id = endian_swap2(glyph_desc.id);
		glyph_desc.x = endian_swap2(glyph_desc.x);
		glyph_desc.y = endian_swap2(glyph_desc.y);
		glyph_desc.width = endian_swap2(glyph_desc.width);
		glyph_desc.height = endian_swap2(glyph_desc.height);
		glyph_desc.xoffset = endian_swap2(glyph_desc.xoffset);
		glyph_desc.yoffset = endian_swap2(glyph_desc.yoffset);
		glyph_desc.xadvance = endian_swap2(glyph_desc.xadvance);
	
		file_write(f, &glyph_desc, sizeof(GlyphDesc));
	}

	file_close(f);
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

	// Construct texture name
	char* font_name = path_change_ext(filename, "");	
	char texture_name[256];
	assert(strlen(font_name) + 10 < 256);
	sprintf(texture_name, "%s_%dpx.tga", font_name, px_size);
	MEM_FREE(font_name);

	mkfnt_bake(size, texture_name);
	mkfnt_write_bft(texture_name);

	return 0;
}

