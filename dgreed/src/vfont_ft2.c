#include "vfont.h"

#include "lib9-utf/utf.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "darray.h"
#include "memory.h"

DArray fonts;
bool vfont_retina = false;
uint vfont_selected_font;

static FT_Library library;

void _vfont_init(void) {
	fonts = darray_create(sizeof(Font), 0);
	FT_Init_FreeType(&library);
}

void _vfont_close(void) {
	FT_Done_FreeType(library);

	// Free fonts
	for(uint i = 0; i < fonts.size; ++i) {
		Font* font = darray_get(&fonts, i);
		MEM_FREE(font->name);
	}
	darray_free(&fonts);
}

RectF _vfont_bbox(const char* string) {
    int error = 0;
    Font* font = darray_get(&fonts, vfont_selected_font);
    
    int has_kerning = font->face->face_flags & FT_FACE_FLAG_KERNING;
    int width = 0;
    int height = 0;
    int yMax = 0;
    int yMin = 0; 
    int left_index;
    int current_index = 0;
    uint i = 0;
    Rune codepoint;
    int n = strlen(string);
    while(i < n) {
	i += chartorune(&codepoint, string + i);
	left_index = current_index;
	current_index = FT_Get_Char_Index(font->face, codepoint);

	error = FT_Load_Glyph(font->face, current_index, FT_LOAD_DEFAULT);
	if(error)
	    LOG_ERROR("Failed to load glyph no. %d", current_index);
	
	FT_BBox bbox;
	FT_Glyph glyph;
	error = FT_Get_Glyph(font->face->glyph, &glyph);
	if(error)
	    LOG_ERROR("Failed to get glyph");
	FT_Glyph_Get_CBox(glyph, 0, &bbox);
	if(yMax < bbox.yMax)
	    yMax = bbox.yMax;
	if(yMin > bbox.yMin)
	    yMin = bbox.yMin;

	if(font->face->glyph->metrics.width < font->face->glyph->advance.x)
	    width += font->face->glyph->advance.x;
	else
	    width += font->face->glyph->metrics.width;

	if(left_index && current_index && has_kerning) {
	    FT_Vector kerning;
	    error = FT_Get_Kerning(font->face, left_index, current_index,
		FT_KERNING_DEFAULT, &kerning);
	    if(error)
		LOG_ERROR("Failed to get kerning between %d and %d",
		    left_index, current_index);
	    width += kerning.x;
        }

    }
    height = yMax - yMin;
    width /= 64;
    height /= 64;
    return rectf(0.0f, 0.0f, width, height);
}

void _vfont_render_text(const char* string, CachePage* page, RectF* dest) {
    uint x = (uint)ceilf(dest->left);
    uint y = (uint)ceilf(dest->top);
    uint w = (uint)ceilf(rectf_width(dest));
    uint h = (uint)ceilf(rectf_height(dest));
    int error;
    Font* font = darray_get(&fonts, vfont_selected_font);

    FT_GlyphSlot slot = font->face->glyph;
    FT_UInt current;
    FT_UInt previous;

    FT_BitmapGlyph g_bitmap;

    int use_kerning = FT_HAS_KERNING(font->face);
    LOG_INFO("Kerning is %savailable.",use_kerning?"":"not ");
    previous = 0;
    
    int baseline = 0;
    uint i = 0;
    Rune codepoint;
    int n = strlen(string);
    while(i < n) {
	i += chartorune(&codepoint, string + i);
	current = FT_Get_Char_Index(font->face, codepoint);
	FT_Load_Glyph(font->face, current,0);
	int height = font->face->glyph->metrics.horiBearingY;
	if(height > baseline)
	    baseline = height;

    }
    baseline /= 64;
    
    // Render everything, then blit.    
    Color *pix = malloc(sizeof(pix) * w * h);

    uint pix_x = 0;
    uint pix_y = 0;
    uint pix_h = h;
    uint pix_w = w;
    memset(pix, COLOR_RGBA(0,0,0,255), w * h * sizeof(pix));
    tex_blit(page->tex, pix, x, y, pix_w, pix_h);
   
    LOG_INFO("word w: %d h: %d", w, h); 
    i = 0;
    while(i < n) {
	i += chartorune(&codepoint, string + i);
	current = FT_Get_Char_Index(font->face, codepoint);
	if (use_kerning && previous && current) {
	    FT_Vector k;
	    FT_Get_Kerning(font->face, previous, current, FT_KERNING_DEFAULT, &k);
	    pix_x += k.x/64;
	    pix_y += k.y/64;
	}
	FT_Load_Glyph(font->face, current, FT_LOAD_RENDER);

	FT_Glyph glyph;

	error = FT_Get_Glyph(slot, &glyph);
	if (error)
	    LOG_ERROR("Failed to get glyph");

	if(glyph->format != FT_GLYPH_FORMAT_BITMAP){
	    error = FT_Glyph_To_Bitmap(&glyph, 0, 0, 0);
	    if (error)
		LOG_ERROR("Failed to connvert glyph to bitmap");
	}
	
	g_bitmap = (FT_BitmapGlyph) glyph;
	uint bmp_w = g_bitmap->bitmap.width;
	uint bmp_h = g_bitmap->bitmap.rows;
	int render_y = baseline - slot->metrics.horiBearingY/64;
	 
	int j = 0;
	for(int r = 0; r < bmp_h; r++) {
	    for(int k = pix_x; k < (pix_x + bmp_w); k++, j++) {
		pix[(render_y + r) * pix_w + k] |= COLOR_RGBA(255, 255, 255,
		     g_bitmap->bitmap.buffer[j]);
		}
	}   	    			
	pix_x += slot->advance.x/64;
	previous = current;
    }
	
    tex_blit(page->tex, pix, x, y, pix_w, pix_h);
    free(pix);

}

void vfont_select(const char* font_name, float size) {
    assert(font_name && size);

    // Look for existing font
    Font* f = DARRAY_DATA_PTR(fonts, Font);
    for(uint i = 0; i < fonts.size; ++i) {
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
    int error = 0;
    error = FT_New_Face(library, new.name, 0, &new.face);
    if(error)
        LOG_ERROR("Failed to create new face err no.%d font name: %s, lib: %u, face: %u",
            error, new.name, library, &new.face);

    error = FT_Select_Charmap(new.face, FT_ENCODING_UNICODE);
    error = FT_Set_Char_Size(new.face, size * 64, size * 64, 72, 72); // 100-dpi  
    if(error)
        LOG_ERROR("Failed to set character size");

    darray_append(&fonts, &new);
    vfont_selected_font = fonts.size - 1;
}

