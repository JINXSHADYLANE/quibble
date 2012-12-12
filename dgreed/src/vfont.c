#include "vfont.h"
#include "lib9-utf/utf.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "image.h"
#include "darray.h"
#include "datastruct.h"
#include "memory.h"

typedef struct {
    const char* name;
    float size;
    FT_Face face;
} Font;

typedef struct {
    float width, height;
    RectF occupied;
    TexHandle tex;
} CachePage;

typedef struct {
    const char* text;
    const char* font_name;
    float size;

    TexHandle tex;
    RectF src;
    CachePage* page;
} CachedText;

typedef struct {
    CachePage* page;
    RectF free;
} FreeRect;

static uint default_page_width = 512;
static uint default_page_height = 512;

static Dict cache;
static DArray fonts;
static DArray cache_pages;
static DArray free_rects;

static uint selected_font = 0;
static FT_Library library;

static RectF _bbox(const char* string) {
    int error = 0;
    Font* font = darray_get(&fonts, selected_font);
    
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

static CachePage* _alloc_page(RectF* r) {
    assert(r->left == 0.0f && r->top == 0.0f);

    // Determine page size
    uint pw = default_page_width;
    uint ph = default_page_height;
    if(pw < r->right)
        pw = next_pow2((uint)r->right);
    if(ph < r->bottom)
        ph = next_pow2((uint)r->bottom);

    // Fill page struct
    CachePage new = {
        .width = (float)pw,
        .height = (float)ph,
        .occupied = *r,
        .tex = tex_create(pw, ph)
    };

    // Alloc new page
    darray_append(&cache_pages, &new);

	LOG_INFO("Allocated %dx%d vfont cache page.", pw, ph);

    return darray_get(&cache_pages, cache_pages.size-1);
}

static CachePage* _alloc_cache(RectF* r) {
    assert(r->left == 0.0f && r->top == 0.0f);
    float w = r->right;
    float h = r->bottom;

    // Try to fit into one of existing pages
    for(uint i = 0; i < cache_pages.size; ++i) {
        CachePage* page = darray_get(&cache_pages, i);
        float ow = rectf_width(&page->occupied);
        float oh = rectf_height(&page->occupied);

        // Append to right
        if(ow + w < page->width && h <= page->height) {
            r->left = page->occupied.right;
            r->right += r->left;

            page->occupied.right += w;
            page->occupied.bottom = MAX(page->occupied.bottom, r->bottom);

            FreeRect new = {
                .page = page,
                .free = {
                    .left = r->left,
                    .top = r->bottom,
                    .right = r->right,
                    .bottom = page->occupied.bottom
                }
            };

            // Append new rect only if it's higher than
            // the rect we're allocating space for
            if(rectf_height(&new.free) >= h)
                darray_append(&free_rects, &new);

            return page;
        }

        // Append to bottom
        if(oh + h < page->height && w <= page->width) {
            r->top = page->occupied.bottom;
            r->bottom += r->top;

            FreeRect new = {
                .page = page,
                .free = {
                    .left = r->right,
                    .top = r->top,
                    .right = page->occupied.right,
                    .bottom = r->bottom
                }
            };
            darray_append(&free_rects, &new);
            page->occupied.bottom += h;
            return page;
        }
    }

    // Alloc completely new page
    return _alloc_page(r);
}

static void _render_text(const char* string, CachePage* page, RectF* dest) {
    uint x = (uint)ceilf(dest->left);
    uint y = (uint)ceilf(dest->top);
    uint w = (uint)ceilf(rectf_width(dest));
    uint h = (uint)ceilf(rectf_height(dest));
    int error;
    Font* font = darray_get(&fonts, selected_font);

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

static const CachedText* _precache(const char* string, const char* key, bool input) {
    RectF bbox = _bbox(string);
    float w = rectf_width(&bbox);
    float h = rectf_height(&bbox);

    // Make user inputed text very long, so that we can use cache better
    if(input) {
        float input_w =256.0f;
        assert(w < input_w);
        w = input_w;
        bbox.right = input_w;
    }

    // Look for space in free rects, choose smallest that fits
    FreeRect* frects = DARRAY_DATA_PTR(free_rects, FreeRect);
    FreeRect* choice = NULL;
    uint choice_i;
    float choice_w, choice_h;
    for(uint i = 0; i < free_rects.size; ++i) {
        float fw = rectf_width(&frects[i].free);
        float fh = rectf_height(&frects[i].free);

        if(w <= fw && h <= fh) {
            if(!choice || fw < choice_w) {
                choice = &frects[i];
                choice_i = i;
                choice_w = fw;
                choice_h = fh;
            }
        }
    }

    CachePage* page = NULL;

    // If we're using less than a 90% width/height of free rect - mark unused space;
    // otherwise - assume all free space was used
    if(choice) {

        page = choice->page;
        bbox.left = choice->free.left;
        bbox.top = choice->free.top;
        bbox.right += bbox.left;
        bbox.bottom += bbox.top;

        if(w < choice_w * 0.9f) {
            choice->free.left += w;
        }
        else if(h < choice_h * 0.9f) {
            choice->free.top += h;
        }
        else {
            darray_remove_fast(&free_rects, choice_i);
        }
    }
    else {
        page = _alloc_cache(&bbox);
    }

    assert(page);

    // Rasterize
    _render_text(string, page, &bbox);

    // Fill text cache entry
    Font* font = darray_get(&fonts, selected_font);
    CachedText new = {
    	.text = strclone(string),
		.font_name = font->name,
		.size = font->size,
		.tex = page->tex,
		.src = bbox,
        .page = page
    };

	CachedText* hnew = MEM_ALLOC(sizeof(CachedText));
	memcpy(hnew, &new, sizeof(CachedText));

	dict_set(&cache, strclone(key), hnew);

	return hnew;
}

#define _key(str) \
    Font* font = darray_get(&fonts, selected_font); \
    size_t text_len = strlen(str); \
    size_t font_len = strlen(font->name); \
    char* key = alloca(text_len + font_len + 16); \
    sprintf(key, "%s:%s:%f", str, font->name, font->size)


static const CachedText* _get_text(const char* string, bool input) {
    // Construct cache key
	_key(string);

    // Return cached text
    const CachedText* text = dict_get(&cache, key);
    if(text)
        return text;

    // Text was not in cache, rasterize
    return _precache(string, key, input);
}

void vfont_init(void) {
	vfont_init_ex(512, 512);
	int error = FT_Init_FreeType(&library);
	if(error)
	    LOG_ERROR("Failed to init FreeType");

}

void vfont_init_ex(uint cache_w, uint cache_h) {
	default_page_width = cache_w;
	default_page_height = cache_h;

	dict_init(&cache);
	fonts = darray_create(sizeof(Font), 0);
	cache_pages = darray_create(sizeof(CachePage), 0);
	free_rects = darray_create(sizeof(FreeRect), 0);

}

void vfont_close(void) {
	darray_free(&free_rects);

	// Free texture pages
	for(uint i = 0; i < cache_pages.size; ++i) {
		CachePage* page = darray_get(&cache_pages, i);
		tex_free(page->tex);
	}
	darray_free(&cache_pages);

	// Free fonts
	for(uint i = 0; i < fonts.size; ++i) {
		Font* font = darray_get(&fonts, i);
		MEM_FREE(font->name);
	}
	darray_free(&fonts);

	// Free cached texts
	for(uint i = 0; i < cache.mask+1; ++i) {
		DictEntry e = cache.map[i];
		if(e.key && e.data) {
			const CachedText* text = e.data;
			MEM_FREE(text->text);
			MEM_FREE(e.data);
			MEM_FREE(e.key);
		}
	}
	dict_free(&cache);
}

void vfont_select(const char* font_name, float size) {
    assert(font_name && size);

    // Look for existing font
    Font* f = DARRAY_DATA_PTR(fonts, Font);
    for(uint i = 0; i < fonts.size; ++i) {
        if(size == f[i].size && strcmp(font_name, f[i].name) == 0) {
            selected_font = i;
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
    selected_font = fonts.size - 1;
}

void vfont_draw(const char* string, uint layer, Vector2 topleft, Color tint) {
    assert(string);

    if(strcmp(string, "") == 0)
        return;

    const CachedText* text = _get_text(string, false);
    RectF dest = rectf(topleft.x, topleft.y, 0.0f, 0.0f);
    video_draw_rect(text->tex, layer, &text->src, &dest, tint);
}

void vfont_draw_input(const char* string, uint layer, Vector2 topleft, Color tint) {
    assert(string);

    if(strcmp(string, "") == 0)
        return;

    static char prev_string[256] = "";
    if(strcmp(string, prev_string) == 0) {
        vfont_draw(string, layer, topleft, tint);
        return;
    }

    if(strcmp(prev_string, "") != 0)
        vfont_cache_invalidate_ex(prev_string, false);
    strcpy(prev_string, string);
    _get_text(string, true);

    vfont_draw(string, layer, topleft, tint);
}

void vfont_precache(const char* string) {
    if(strcmp(string, "") == 0)
        return;
    _get_text(string, false);
}

void vfont_cache_invalidate(const char* string) {
    vfont_cache_invalidate_ex(string, true);
}

void vfont_cache_invalidate_ex(const char* string, bool strict) {
	_key(string);

    if(strcmp(string, "") == 0)
        return;

	DictEntry* text_entry = dict_entry(&cache, key);
	if(!text_entry && strict)
		LOG_ERROR("Invalidating not-precached text");
    if(!text_entry && !strict)
        return;

	const CachedText* text = (const CachedText*)text_entry->data;
	assert(text);

	// Mark new free rect
	FreeRect new = {
		.page = text->page,
		.free = text->src
	};
	darray_append(&free_rects, &new);

	// Free memory
	MEM_FREE(text->text);
	MEM_FREE(text);
	const char* key_text = text_entry->key;

	// Remove cache entry
	dict_delete(&cache, key);

	MEM_FREE(key_text);
}

Vector2 vfont_size(const char* string) {
    RectF bbox = _bbox(string);
    Vector2 res = {
        .x = rectf_width(&bbox),
        .y = rectf_height(&bbox)
    };
    return res;
}

void vfont_draw_cache(uint layer, Vector2 topleft) {
	if(cache_pages.size > 0) {
		CachePage* page = darray_get(&cache_pages, 0);
		RectF dest = rectf(topleft.x, topleft.y, 0.0f, 0.0f);
		video_draw_rect(page->tex, layer, NULL, &dest, COLOR_WHITE);
	}
}

