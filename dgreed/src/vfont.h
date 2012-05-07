#ifndef VFONT_H
#define VFONT_H

#include "utils.h"
#include "system.h"

// Slow and beautiful text renderer
// Uses CoreText on iOS/OS X and Freetype 2 everywhere else
//
// Can be quite fast if you're drawing static strings!

void vfont_init(void);
void vfont_init_ex(uint cache_w, uint cache_h);
void vfont_close(void);

void vfont_select(const char* font_name, uint size);
void vfont_draw(const char* string, uint layer, Vector2 topleft, Color tint);
void vfont_precache(const char* string);
void vfont_cache_invalidate(const char* string);

float vfont_width(const char* string);
float vfont_height(void);

void vfont_draw_cache(uint layer, Vector2 topleft);

#endif
