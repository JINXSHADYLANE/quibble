#include "gfx_utils.h"
#include "memory.h"

void gfx_draw_point(uint layer, const Vector2* pos, Color color) {
	assert(pos);

	float offset = 5.0f;
	Vector2 start = vec2(pos->x - offset, pos->y - offset);
	Vector2 end = vec2(pos->x + offset, pos->y + offset);
	video_draw_line(layer, &start, &end, color);
	start.x += 2.0f * offset;
	end.x -= 2.0f * offset;
	video_draw_line(layer, &start, &end, color);
}	

void gfx_draw_rect(uint layer, const RectF* rect, Color color) {
	assert(rect);

	Vector2 p1 = vec2(rect->left, rect->top);
	Vector2 p2 = vec2(rect->right, rect->top);
	Vector2 p3 = vec2(rect->right, rect->bottom);
	Vector2 p4 = vec2(rect->left, rect->bottom);

	video_draw_line(layer, &p1, &p2, color);
	video_draw_line(layer, &p2, &p3, color);
	video_draw_line(layer, &p3, &p4, color);
	video_draw_line(layer, &p4, &p1, color);
}

void gfx_draw_poly(uint layer, const Vector2* points, uint n_points, Color color) {
	assert(points);
	assert(n_points);

	uint i;
	for(i = 1; i < n_points; ++i) 
		video_draw_line(layer, &points[i-1], &points[i], color);
	video_draw_line(layer, &points[n_points-1], &points[0], color);	
}	

void gfx_draw_tri(uint layer, const Triangle* tri, Color color) {
	assert(tri);

	Vector2 points[3] = {tri->p1, tri->p2, tri->p3};
	gfx_draw_poly(layer, points, 3, color);
}	

void gfx_draw_textured_rect(TexHandle tex, uint layer, const RectF* source,
	const Vector2* dest, float rotation, float scale, Color tint) {
	assert(dest);

	RectF src;
	if(source) {
		src = *source;
	}
	else {
		uint w, h;
		tex_size(tex, &w, &h);
		src = rectf(0.0f, 0.0f, (float)w, (float)h);
	}	

	float width = (src.right - src.left) * scale;
	float height = (src.bottom - src.top) * scale;
	RectF dst = rectf(dest->x - width/2.0f, dest->y - height/2.0f,
		dest->x + width/2.0f, dest->y + height/2.0f);

	video_draw_rect_rotated(tex, layer, &src, &dst, rotation, tint);
}	
		

void gfx_transform(Vector2* v, uint n_v, const Vector2* translate, float rotate, 
	float scale)
{
	assert(v);
	assert(n_v);
	assert(translate);

	// Scale
	uint i;
	for(i = 0; i < n_v; ++i)
		v[i] = vec2_scale(v[i], scale);	

	// Rotate
	float s = sin(rotate * DEG_TO_RAD);
	float c = cos(rotate * DEG_TO_RAD);
	for(i = 0; i < n_v; ++i) {
		float nx, ny;
		nx = c * v[i].x - s * v[i].y;
		ny = s * v[i].x + c * v[i].y;
		v[i].x = nx; v[i].y = ny;
	}	

	// Translate
	for(i = 0; i < n_v; ++i) 
		v[i] = vec2_add(v[i], *translate);	
}		

Color gfx_sample_img(Color* img, uint w, uint h, int x, int y) {
	assert(img);

	// Clamp coords
	x = MIN(MAX(x, 0), (int)w-1);
	y = MIN(MAX(y, 0), (int)h-1);

	// Sample
	return img[IDX_2D(x, y, (int)w)];
}	

const int gaussian_shifts[] = {-3, -2, -1, 0, 1, 2, 3};
const uint gaussian_kernel[] = {2, 4, 8, 14, 8, 4, 2};
const uint gaussian_div = 42;
const int gaussian_width = ARRAY_SIZE(gaussian_kernel);

Color _sample_gaussian(Color* img, uint w, uint h, int x, int y, bool horiz) {
	byte br, bg, bb, ba;
	uint r = 0, g = 0, b = 0, a = 0;
	Color sample;

	for(uint i = 0; i < gaussian_width; ++i) {
		int shift = gaussian_shifts[i];
		uint coeff = gaussian_kernel[i];

		if(horiz)
			sample = gfx_sample_img(img, w, h, x + shift, y);
		else
			sample = gfx_sample_img(img, w, h, x, y + shift);

		COLOR_DECONSTRUCT(sample, br, bg, bb, ba);
		r += br * coeff; g += bg * coeff; 
		b += bb * coeff; a += ba * coeff;
	}
	r /= gaussian_div; g /= gaussian_div;
	b /= gaussian_div; a /= gaussian_div;

	assert(r < 256); assert(g < 256);
	assert(b < 256); assert(a < 256);

	return COLOR_RGBA(r, g, b, a);
}	

void gfx_blur(Color* img, uint w, uint h) {
	assert(img);
	assert(ARRAY_SIZE(gaussian_shifts) == ARRAY_SIZE(gaussian_kernel));
	// Kernel width must be odd number
	assert(gaussian_width % 2 == 1); 

	Color* temp_img = (Color*)MEM_ALLOC(sizeof(Color) * w * h);

	// Do horizontal pass
	for(int y = 0; y < (int)h; ++y) 
		for(int x = 0; x < (int)w; ++x) 
			temp_img[IDX_2D(x, y, w)] = _sample_gaussian(img, w, h, x, y, true);
	
	// Do vertical pass
	for(int y = 0; y < (int)h; ++y)
		for(int x = 0; x < (int)w; ++x)
			img[IDX_2D(x, y, w)] = _sample_gaussian(temp_img, w, h, x, y, false);

	MEM_FREE(temp_img);
}

Color gfx_blend(Color ca, Color cb) {
	uint ar, ag, ab, aa;
	uint br, bg, bb, ba;
	uint r, g, b, a;

	COLOR_DECONSTRUCT(ca, ar, ag, ab, aa);
	COLOR_DECONSTRUCT(cb, br, bg, bb, ba);

	r = (ar * (255-ba) + br * ba) / 255;
	g = (ag * (255-ba) + bg * ba) / 255;
	b = (ab * (255-ba) + bb * ba) / 255;
	a = aa;

	assert(r < 256); assert(g < 256);
	assert(b < 256); assert(a < 256);

	return COLOR_RGBA(r, g, b, a);
}


