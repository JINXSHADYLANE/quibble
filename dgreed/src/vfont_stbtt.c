#include "vfont.h"

#include "lib9-utf/utf.h"

#include "darray.h"
#include "memory.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_ifloor(x) ((int) floorf(x))
#define STBTT_iceil(x) ((int) ceilf(x))
#define STBTT_sort(data,num_items,item_size,compare_func) \
	sort_heapsort(data,num_items,item_size,compare_func)
#define STBTT_malloc(x,u) MEM_ALLOC(x)
#define STBTT_free(x,u) MEM_FREE(x)
#include "stb_truetype.h"

DArray vfont_fonts;
bool vfont_retina = false;
uint vfont_selected_font;

void _vfont_init(void) {
	vfont_fonts = darray_create(sizeof(Font), 0);
}

void _vfont_close(void) {
	// Free vfont_fonts
	for(uint i = 0; i < vfont_fonts.size; ++i) {
		Font* font = darray_get(&vfont_fonts, i);
		MEM_FREE(font->font.data);
		MEM_FREE(font->name);
	}
	darray_free(&vfont_fonts);
}

RectF _vfont_bbox(const char* string) {
    Font* font = darray_get(&vfont_fonts, vfont_selected_font);

	float xpos = 0.0f;

	int ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&font->font, &ascent, &descent, &line_gap);

	int i = 0;
    int n = strlen(string);
	uint codepoint = 0;
	int glyph;
	while(i < n) {
		int advance, lsb, next_glyph;
		uint next_codepoint;
		i += chartorune(&next_codepoint, string + i);
		next_glyph = stbtt_FindGlyphIndex(&font->font, next_codepoint);

		if(codepoint && next_codepoint)
			xpos += font->scale * stbtt_GetGlyphKernAdvance(
				&font->font, glyph, next_glyph
			);

		codepoint = next_codepoint;
		glyph = next_glyph;

		stbtt_GetGlyphHMetrics(&font->font, glyph, &advance, &lsb);
		xpos += (advance * font->scale);
	}

    return rectf(0.0f, 0.0f, STBTT_iceil(xpos), 
		STBTT_iceil((ascent - descent) * font->scale)
	);
}

void _vfont_render_text(const char* string, CachePage* page, RectF* dest) {
    Font* font = darray_get(&vfont_fonts, vfont_selected_font);

    uint x = (uint)dest->left;
    uint y = (uint)dest->top;
    uint w = (uint)rectf_width(dest);
    uint h = (uint)rectf_height(dest);

 	float xpos = 0.0f;

	int ascent, descent, line_gap, baseline;
	stbtt_GetFontVMetrics(&font->font, &ascent, &descent, &line_gap);
	baseline = (int) (ascent * font->scale);
	
    Color *pix = MEM_ALLOC(sizeof(Color) * w * h);
	memset(pix, 0, w * h * sizeof(Color));

	byte* temp = MEM_ALLOC(h * h * 2);

	int i = 0;
    int n = strlen(string);
	uint codepoint = 0;
	int glyph;
	while(i < n) {
		uint next_codepoint;
		int advance, lsb, x0, y0, x1, y1, next_glyph;
		float kerning = 0.0f;
		i += chartorune(&next_codepoint, string + i);
		next_glyph = stbtt_FindGlyphIndex(&font->font, next_codepoint);

		if(codepoint && next_codepoint)
			kerning = stbtt_GetGlyphKernAdvance(
				&font->font, glyph, next_glyph
			);

		codepoint = next_codepoint;
		glyph = next_glyph;

		float x_shift = xpos - floorf(xpos);

		stbtt_GetGlyphHMetrics(&font->font, glyph, &advance, &lsb);

		stbtt_GetGlyphBitmapBoxSubpixel(
			&font->font, glyph, font->scale, font->scale, x_shift, 0, 
			&x0, &y0, &x1, &y1
		);

		assert((x1 - x0) <= h * 2);

		stbtt_MakeGlyphBitmapSubpixel(
			&font->font, temp, x1-x0, y1-y0, x1-x0, font->scale, font->scale,
			x_shift, 0.0f, glyph
		);

		// Blend glyph into pix buffer
		int sx = (int)(xpos + lsb * font->scale);
		int sy = baseline + y0;
		for(int y = 0; y < (y1-y0); ++y) {
			for(int x = 0; x < (x1-x0); ++x) {
				byte c = temp[y * (x1-x0) + x];

			#if defined(TARGET_IOS) || defined(ANDROID)
				// Premultiply alpha
				pix[(sy + y) * w + (sx + x)] |= COLOR_RGBA(c, c, c, c);
			#else
				// Don't premultiply alpha
				pix[(sy + y) * w + (sx + x)] |= COLOR_RGBA(255, 255, 255, c);
			#endif
			}
		}

		xpos += ((advance + kerning) * font->scale);
	}
	MEM_FREE(temp);
  
    tex_blit(page->tex, pix, x, y, w, h);
    MEM_FREE(pix);
}

void vfont_select(const char* font_name, float size) {
    assert(font_name && size);

    // Look for existing font
    Font* f = DARRAY_DATA_PTR(vfont_fonts, Font);
    for(uint i = 0; i < vfont_fonts.size; ++i) {
        if(size == f[i].size && strcmp(font_name, f[i].name) == 0) {
            vfont_selected_font = i;
            return;
        }
    }

    // Make new font
    Font new = {
        .name = strclone(font_name),
        .size = size,
    };

	FileHandle file = file_open(font_name);
    size_t fs = file_size(file);
    void* fd = MEM_ALLOC(fs);
    file_read(file, fd, fs);
    file_close(file);

	if(!stbtt_InitFont(&new.font, fd, 0))
		LOG_ERROR("Unable to use font %s", font_name);

	new.scale = stbtt_ScaleForPixelHeight(&new.font, STBTT_iceil(size * 1.1f));

    darray_append(&vfont_fonts, &new);
    vfont_selected_font = vfont_fonts.size - 1;
}

