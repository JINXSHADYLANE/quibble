#include "vfont.h"
#include "darray.h"
#include "datastruct.h"
#include "memory.h"
#include "mempool.h"

static uint default_page_width = 512;
static uint default_page_height = 512;

static Dict cache;
static DArray cache_pages;
static DArray free_rects;

static MemPool cached_text_pool;
static MemPool key_str_pool;

// System-specific render implementation interface
extern uint vfont_selected_font;
extern bool vfont_retina;
extern void _vfont_init(void);
extern void _vfont_close(void);
extern RectF _vfont_bbox(const char* string);
extern void _vfont_render_text(const char* string, CachePage* page, RectF* dest);

extern DArray fonts;

static CachePage* _alloc_page(RectF* r) {
    assert(r->left == 0.0f && r->top == 0.0f);
    
    // Determine page size
    uint pw = default_page_width * (vfont_retina ? 2 : 1);
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

static const CachedText* _precache(const char* string, const char* key, bool input) {
    RectF bbox = _vfont_bbox(string);
    if(vfont_retina) {
        bbox.right *= 2.0f;
        bbox.bottom *= 2.0f;
    }
    float w = rectf_width(&bbox);
    float h = rectf_height(&bbox);
    
    // Make user inputed text very long, so that we can use cache better
    if(input) {
        float input_w = vfont_retina ? 510.0f * 2.0f : 510.0f;
        // Don't crash, log warning instead
		if(w >= input_w)
        	LOG_WARNING("Input text w is longer than it should be!");
        //assert(w < input_w);
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
    _vfont_render_text(string, page, &bbox);
    
    // Fill text cache entry
    Font* font = darray_get(&fonts, vfont_selected_font);

	CachedText* hnew = mempool_alloc(&cached_text_pool);
	strcpy(hnew->text, string);
	hnew->font_name = font->name;
	hnew->size = font->size;
	hnew->tex = page->tex;
	hnew->src = bbox;
	hnew->page = page; 

	assert(strlen(key) < 128);
	char* pkey = mempool_alloc(&key_str_pool);
	strcpy(pkey, key);
	dict_set(&cache, pkey, hnew); 

	return hnew;
}

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
}

void vfont_init_ex(uint cache_w, uint cache_h) {
	default_page_width = cache_w;
	default_page_height = cache_h;

	dict_init(&cache);
	cache_pages = darray_create(sizeof(CachePage), 0);
	free_rects = darray_create(sizeof(FreeRect), 0);

	mempool_init_ex(&cached_text_pool, sizeof(CachedText), 8*1024);
	mempool_init_ex(&key_str_pool, 128, 4*1024);

	_vfont_init();
}

void vfont_close(void) {
	_vfont_close();

	darray_free(&free_rects);

	// Free texture pages
	for(uint i = 0; i < cache_pages.size; ++i) {
		CachePage* page = darray_get(&cache_pages, i);
		tex_free(page->tex);
	}
	darray_free(&cache_pages);
	// Free cached texts
	dict_free(&cache);

	mempool_drain(&key_str_pool);
	mempool_drain(&cached_text_pool);
}

void vfont_draw(const char* string, uint layer, Vector2 topleft, Color tint) {
    assert(string);
    
    if(strcmp(string, "") == 0)
        return;
    
    const CachedText* text = _get_text(string, false);
    RectF dest = rectf(floorf(topleft.x), floorf(topleft.y), 0.0f, 0.0f);
    if(vfont_retina) {
        dest.right = dest.left + rectf_width(&text->src) / 2.0f;
        dest.bottom = dest.top + rectf_height(&text->src) / 2.0f;
    }
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
	if(!text_entry && strict) {
		LOG_WARNING("Invalidating not-precached text");
        return;
    }
    if(!text_entry && !strict)
        return;

	assert(text_entry);
    CachedText* text = (CachedText*)text_entry->data;
	assert(text);

	// Mark new free rect
	FreeRect new = {
		.page = text->page,
		.free = text->src
	};
	darray_append(&free_rects, &new);

	// Free memory
	mempool_free(&cached_text_pool, text);
	char* key_text = (char*)text_entry->key;

	// Remove cache entry
	dict_delete(&cache, key);

	mempool_free(&key_str_pool, key_text);
}

void vfont_invalidate_all(void) {
    free_rects.size = 0;

    dict_free(&cache);
    dict_init(&cache);

	mempool_free_all(&key_str_pool);
	mempool_free_all(&cached_text_pool);

    for(uint i = 0; i < cache_pages.size; ++i) {
        CachePage* page = darray_get(&cache_pages, i);
        page->occupied = rectf_null();
    }
}

Vector2 vfont_size(const char* string) {
    RectF bbox = _vfont_bbox(string);
    Vector2 res = {
        .x = rectf_width(&bbox),
        .y = rectf_height(&bbox)
    };
    return res;
}

void vfont_draw_cache(uint layer, Vector2 topleft) {
    for(uint i = 0; i < cache_pages.size; ++i) {
		CachePage* page = darray_get(&cache_pages, i);
		RectF dest = rectf(topleft.x, topleft.y, 0.0f, 0.0f);
		video_draw_rect(page->tex, layer, NULL, &dest, COLOR_WHITE);
        topleft.y += page->height;
	}
}

