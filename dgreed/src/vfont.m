#include "vfont.h"

#import <QuartzCore/QuartzCore.h>

#ifdef TARGET_IOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#include "darray.h"
#include "datastruct.h"
#include "memory.h"

typedef struct {
    const char* name;
    float size;
#ifdef TARGET_IOS
    UIFont* uifont;
#else
	NSFont* uifont;
#endif
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

static bool retina = false;

static uint default_page_width = 512;
static uint default_page_height = 512;

static Dict cache;
static DArray fonts;
static DArray cache_pages;
static DArray free_rects;

static uint selected_font = 0;

static RectF _bbox(const char* string) {
    Font* font = darray_get(&fonts, selected_font);
    NSString* ns_string = [NSString stringWithUTF8String:string];

#ifdef TARGET_IOS
    CGSize size = [ns_string sizeWithFont:font->uifont];
#else
	NSDictionary* attr = [NSDictionary dictionaryWithObject:font->uifont forKey: NSFontAttributeName];
	NSSize size = [ns_string sizeWithAttributes:attr];
#endif

    return rectf(0.0f, 0.0f, size.width, size.height);
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
        
            FreeRect new = {
                .page = page,
                .free = {
                    .left = r->left,
                    .top = r->bottom,
                    .right = r->right,
                    .bottom = page->occupied.bottom
                }
            };
            darray_append(&free_rects, &new);
            page->occupied.right += w;
            
            return page;
        }
        
        // Append to bottom
        if(oh + h < page->width && w <= page->width) {
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
    
    Color* data = MEM_ALLOC(w * h * sizeof(Color));
    memset(data, 0, w * h * sizeof(Color));
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        data, w, h, 8, w*4, 
        color_space, kCGImageAlphaPremultipliedLast
    );

#ifdef TARGET_IOS
    UIGraphicsPushContext(ctx);
#else
	NSGraphicsContext *old_nsctx = [NSGraphicsContext currentContext];
	NSGraphicsContext *nsctx = [NSGraphicsContext graphicsContextWithGraphicsPort:ctx flipped:NO];
	[NSGraphicsContext setCurrentContext:nsctx];
#endif
    
    CGContextSaveGState(ctx); 

#ifdef TARGET_IOS
    CGContextTranslateCTM(ctx, 0, h);
    if(retina)
        CGContextScaleCTM(ctx, 2.0, -2.0);
    else
        CGContextScaleCTM(ctx, 1.0, -1.0);
#endif
        
    CGFloat c_rgba[] = {1.0f, 1.0f, 1.0f, 1.0f};
    CGColorRef c = CGColorCreate(color_space, c_rgba);
    CGContextSetFillColorWithColor(ctx, c);
    CGColorRelease(c);
    
    Font* font = darray_get(&fonts, selected_font);
    NSString* ns_str = [NSString stringWithUTF8String:string];

#ifdef TARGET_IOS
    [ns_str drawAtPoint:CGPointMake(0.0f, 0.0f) withFont:font->uifont];
#else
	NSPoint pt = {0.0f, 0.0f};
	NSDictionary* attr = [NSDictionary dictionaryWithObjectsAndKeys:
		font->uifont, NSFontAttributeName,
		[NSColor whiteColor], NSForegroundColorAttributeName,
		nil];
	[ns_str drawAtPoint:pt withAttributes:attr];
#endif
    
    /*
    // Premultiply alpha
    for(uint i = 0; i < w * h; ++i) {
        byte r, g, b, a;
        COLOR_DECONSTRUCT(data[i], r, g, b, a);
        r = (r * a) >> 8;
        g = (g * a) >> 8;
        b = (b * a) >> 8;
        data[i] = COLOR_RGBA(r, g, b, a);
    }
    */
    
    tex_blit(page->tex, CGBitmapContextGetData(ctx), x, y, w, h);
    CGContextRestoreGState(ctx);

#ifdef TARGET_IOS
    UIGraphicsPopContext();
#else
	[NSGraphicsContext setCurrentContext:old_nsctx];
#endif

    CGContextRelease(ctx);
    CGColorSpaceRelease(color_space);
    
    MEM_FREE(data);
}

static const CachedText* _precache(const char* string, const char* key, bool input) {
    RectF bbox = _bbox(string);
    if(retina) {
        bbox.right *= 2.0f;
        bbox.bottom *= 2.0f;
    }
    float w = rectf_width(&bbox);
    float h = rectf_height(&bbox);
    
    // Make user inputed text very long, so that we can use cache better
    if(input) {
        float input_w = retina ? 512.0f : 256.0f;
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
}

void vfont_init_ex(uint cache_w, uint cache_h) {
	default_page_width = cache_w;
	default_page_height = cache_h;

	dict_init(&cache);
	fonts = darray_create(sizeof(Font), 0);
	cache_pages = darray_create(sizeof(CachePage), 0);
	free_rects = darray_create(sizeof(FreeRect), 0);
    
#ifdef TARGET_IOS
    UIScreen* main = [UIScreen mainScreen];
    if ([main respondsToSelector:@selector(displayLinkWithTarget:selector:)]){
        if(main.scale == 2.0f)
            retina = true;
    }
#endif
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
		[font->uifont release];
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
    
    NSString* ns_font_name = [NSString stringWithUTF8String:font_name];
    
    // Make new font
    Font new = {
        .name = strclone(font_name),
        .size = size,
#ifdef TARGET_IOS
        .uifont = [UIFont fontWithName:ns_font_name size:size]
#else
		.uifont = [NSFont fontWithName:ns_font_name size:size]
#endif
    };
    
    if(new.uifont == nil)
        LOG_ERROR("Unable to select font %s", font_name);
        
    darray_append(&fonts, &new);
    selected_font = fonts.size - 1;
}

void vfont_draw(const char* string, uint layer, Vector2 topleft, Color tint) {
    assert(string);
    
    if(strcmp(string, "") == 0)
        return;
    
    const CachedText* text = _get_text(string, false);
    RectF dest = rectf(topleft.x, topleft.y, 0.0f, 0.0f);
    if(retina) {
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
	MEM_FREE(text_entry->key);

	// Remove cache entry
	dict_delete(&cache, key);
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
        if(retina) {
            dest.right = dest.left + page->width / 2.0f;
            dest.bottom = dest.top + page->height / 2.0f;
        }

		video_draw_rect(page->tex, layer, NULL, &dest, COLOR_WHITE);
	}
}

