#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "utils.h"
#include "system.h"

// Draws little cross whera a point should be
void gfx_draw_point(uint layer, const Vector2* pos, Color color);
// Draws a wireframe rectangle
void gfx_draw_rect(uint layer, const RectF* rect, Color color);
// Draws closed polygon
void gfx_draw_poly(uint layer, const Vector2* points, uint n_points, Color color);
// Draws triangle
void gfx_draw_tri(uint layer, const Triangle* tri, Color color);
// Draws rotated, scaled, textured rect
// source can be NULL, full texture is used in that case
void gfx_draw_textured_rect(TexHandle tex, uint layer, const RectF* source, 
	const Vector2* dest, float rotation, float scale, Color tint);

// Transforms array of vectors
void gfx_transform(Vector2* v, uint n_v, const Vector2* translate, float rotate, 
	float scale);

// Samples 32bit image at some coordinate. If coordinate is outside of texture
// it is clamped to nearest edge pixel
Color gfx_sample_img(Color* img, uint w, uint h, int x, int y);

// Blurs image using 5x5 gaussian blur kernel
void gfx_blur(Color* img, uint w, uint h);

// Blends two colors with the usual inv-src-alpha blending
Color gfx_blend(Color a, Color b);

#endif

