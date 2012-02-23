#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "utils.h"
#include "system.h"

// Draws little cross where a point should be
void gfx_draw_point(uint layer, const Vector2* pos, Color color);
// Draws a wireframe rectangle
void gfx_draw_rect(uint layer, const RectF* rect, Color color);
// Draws rotated & scaled wireframe rectangle
void gfx_draw_rect_rotscale(uint layer, const RectF* rect, float rot, float scale, Color color);
// Draws closed polygon
void gfx_draw_poly(uint layer, const Vector2* points, uint n_points, Color color);
// Draws triangle
void gfx_draw_tri(uint layer, const Triangle* tri, Color color);
// Draws circle
void gfx_draw_circle(uint layer, const Vector2* c, float r, Color color); 
// Draws circle, allows to specify number of line segments used
void gfx_draw_circle_ex(uint layer, const Vector2* c, float r, 
	Color color, uint segs);
// Draws rotated, scaled, textured rect
// source can be NULL, full texture is used in that case
void gfx_draw_textured_rect(TexHandle tex, uint layer, const RectF* source, 
	const Vector2* dest, float rotation, float scale, Color tint);

// Transforms array of vectors
void gfx_transform(Vector2* v, uint n_v, const Vector2* translate, float rotate, 
	float scale);

// Multiplies array of vectors with 3x2 affine matrix
void gfx_matmul(Vector2* v, uint n_v, float* m);

// Samples 32bit image at some coordinate. If coordinate is outside of texture
// it is clamped to nearest edge pixel
Color gfx_sample_img(Color* img, uint w, uint h, int x, int y);

// Blurs image using 5x5 gaussian blur kernel
void gfx_blur(Color* img, uint w, uint h);

// Blends two colors with the usual inv-src-alpha blending
Color gfx_blend(Color a, Color b);

// Resizes image to one half its linear dimensions
Color* gfx_downscale(const Color* img, uint w, uint h);

// Blits source image to destination, using alpha blending
void gfx_blit(Color* dest, uint dest_w, uint dest_h,
	const Color* src, uint src_w, uint src_h, int x, int y);

// Blits source image to destination, using alpha blending.
// Performs wrap-around source image shift.
void gfx_blit_ex(Color* dest, uint dest_w, uint dest_h,
	const Color* src, uint src_w, uint src_h, int x, int y, int ddx, int ddy);

// Fill rectangle with a color. Rect is not clipped!
void gfx_fill(Color* dest, uint dest_w, uint dest_h,
	Color c, int l, int t, int r, int b);

// Saves image into a 32bpp tga file
void gfx_save_tga(const char* filename, Color* img, uint w, uint h);

// Saves image into a 32bpp png file
//void gfx_save_png(const char* filename, Color* img, uint w, uint h);

#endif

