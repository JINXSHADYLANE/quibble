#ifndef VFONT_H
#define VFONT_H

#include "utils.h"
#include "system.h"

// Slow and beautiful text renderer
// Uses CoreText on iOS/OS X and stb_truetype with subpixel
// glyph positioning everywhere else. Freetype2 implementation
// is also available, but it looks shit.
//
// Can be quite fast if you're drawing static strings!

#if defined(__APPLE__) && !defined(VFONT_STBTT)

typedef struct {
    const char* name;
    float size;
	void* uifont;
} Font;

#elif defined(VFONT_FREETYPE2)

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

typedef struct {
    const char* name;
    float size;
    FT_Face face;
} Font;

#else

#include "stb_truetype.h"

typedef struct {
	const char* name;
	float size;
	float scale;
	stbtt_fontinfo font;
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
    Font* font = darray_get(&vfont_fonts, vfont_selected_font); \
    size_t text_len = strlen(str); \
    size_t font_len = strlen(font->name); \
    char* key = alloca(text_len + font_len + 16); \
    sprintf(key, "%s:%s:%f", str, font->name, font->size)

void vfont_init(void);
void vfont_init_ex(uint cache_w, uint cache_h);
void vfont_close(void);

// Render fonts at 'factor' times higher resolution than virtual
// screen resolution
void vfont_resolution_factor(float factor);

void vfont_select(const char* font_name, float size);
void vfont_draw(const char* string, uint layer, Vector2 topleft, Color tint);
void vfont_draw_ex(const char* string, uint layer, Vector2 topleft, Color tint, float scale);
void vfont_draw_input(const char* string, uint layer, Vector2 topleft, Color tint);
void vfont_draw_number(int number, const char* postfix, uint layer, Vector2 topleft, Color tint);
void vfont_draw_number_ex(int number, const char* postfix, uint layer, Vector2 topleft, Color tint, float scale);
void vfont_precache(const char* string);
void vfont_cache_invalidate(const char* string);
void vfont_cache_invalidate_ex(const char* string, bool strict);
void vfont_invalidate_all(void);

Vector2 vfont_size(const char* string);
Vector2 vfont_number_size(int number);

void vfont_draw_cache(uint layer, Vector2 topleft);

#endif
