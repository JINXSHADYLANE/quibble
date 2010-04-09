#ifndef FONT_H
#define FONT_H

#include "utils.h"
#include "system.h"

typedef uint FontHandle;

// Loads a new font file
FontHandle font_load(const char* filename); 
// Frees font which was loaded with font_load
void font_free(FontHandle font);
// Returns width of text in specified font
float font_width(FontHandle font, const char* string);
// Returns height of font characters in pixels
float font_height(FontHandle font);

// Draws string with specified font
void font_draw(FontHandle font, const char* string, uint layer,
	const Vector2* topleft, Color tint);

#endif
