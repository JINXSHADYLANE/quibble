#ifndef VFONT_H
#define VFONT_H

#include "utils.h"
#include "system.h"

// Slow and beautiful text renderer
// Uses CoreText on iOS/OS X and Freetype 2 everywhere else
//
// Can be quite fast if you're drawing static strings!

#ifdef __APPLE__

typedef struct {
    const char* name;
    float size;
	void* uifont;
} Font;

#else

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

typedef struct {
    const char* name;
    float size;
    FT_Face face;
} Font;

#endif

typedef struct {
    float width, height;
    RectF occupied;
    TexHandle tex;
} CachePage;

typedef struct {
    char text[96];
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

#define _key(str) \
    Font* font = darray_get(&fonts, vfont_selected_font); \
    size_t text_len = strlen(str); \
    size_t font_len = strlen(font->name); \
    char* key = alloca(text_len + font_len + 16); \
    sprintf(key, "%s:%s:%f", str, font->name, font->size)

void vfont_init(void);
void vfont_init_ex(uint cache_w, uint cache_h);
void vfont_close(void);

void vfont_select(const char* font_name, float size);
void vfont_draw(const char* string, uint layer, Vector2 topleft, Color tint);
void vfont_draw_input(const char* string, uint layer, Vector2 topleft, Color tint);
void vfont_precache(const char* string);
void vfont_cache_invalidate(const char* string);
void vfont_cache_invalidate_ex(const char* string, bool strict);
void vfont_invalidate_all(void);

Vector2 vfont_size(const char* string);

void vfont_draw_cache(uint layer, Vector2 topleft);

#endif
