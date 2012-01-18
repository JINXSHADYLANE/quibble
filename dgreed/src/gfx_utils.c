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

void gfx_draw_rect_rotscale(uint layer, const RectF* rect, float rot, 
	float scale, Color color) {

	Vector2 tl = vec2(rect->left, rect->top);
	Vector2 tr = vec2(rect->right, rect->top);
	Vector2 br = vec2(rect->right, rect->bottom);
	Vector2 bl = vec2(rect->left, rect->bottom);
	Vector2 cnt = vec2((rect->left + rect->right) / 2.0f,
		(rect->top + rect->bottom) / 2.0f);

	tl = vec2_add(vec2_scale(vec2_rotate(vec2_sub(tl, cnt), rot), scale), cnt);
	tr = vec2_add(vec2_scale(vec2_rotate(vec2_sub(tr, cnt), rot), scale), cnt);
	br = vec2_add(vec2_scale(vec2_rotate(vec2_sub(br, cnt), rot), scale), cnt);
	bl = vec2_add(vec2_scale(vec2_rotate(vec2_sub(bl, cnt), rot), scale), cnt);

	video_draw_line(layer, &tl, &tr, color);
	video_draw_line(layer, &tr, &br, color);
	video_draw_line(layer, &br, &bl, color);
	video_draw_line(layer, &bl, &tl, color);
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

void gfx_draw_circle(uint layer, const Vector2* c, float r, Color color) {
	// TODO: Cleverly choose segs by r
	gfx_draw_circle_ex(layer, c, r, color, 13);
}

void gfx_draw_circle_ex(uint layer, const Vector2* c, float r, Color color,
	uint segs) {
	assert(c);

	for(uint i = 0; i < segs; ++i) {
		float phi1 = ((float)i / (float)segs) * PI * 2.0f;
		float phi2 = phi1 + (PI * 2.0f / (float)segs);

		Vector2 p1 = vec2(cosf(phi1) * r, sinf(phi1) * r);
		Vector2 p2 = vec2(cosf(phi2) * r, sinf(phi2) * r);
		
		p1 = vec2_add(p1, *c);
		p2 = vec2_add(p2, *c);

		video_draw_line(layer, &p1, &p2, color);
	}
}

void gfx_draw_textured_rect(TexHandle tex, uint layer, const RectF* source,
	const Vector2* dest, float rotation, float scale, Color tint) {
	assert(dest);

	RectF src;
	if(source && (source->right != 0.0f || source->bottom != 0.0f)) {
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
	float s = sin(rotate);
	float c = cos(rotate);
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

Color* gfx_downscale(const Color* img, uint w, uint h) {
	assert(img);
	assert(w && h);
	assert(w % 2 == 0 && h % 2 == 0);

	Color* new_img = (Color*)MEM_ALLOC(sizeof(Color) * w/2 * h/2);

	for(uint y = 0; y < h/2; ++y) {
		for(uint x = 0; x < w/2; ++x) {
			uint r = 0, g = 0, b = 0, a = 0;

			Color samples[4];
			samples[0] = img[IDX_2D(x*2, y*2, w)];
			samples[1] = img[IDX_2D(x*2+1, y*2, w)];
			samples[2] = img[IDX_2D(x*2, y*2+1, w)];
			samples[3] = img[IDX_2D(x*2+1, y*2+1, w)];

			for(uint i = 0; i < 4; ++i) {
				uint sr, sg, sb, sa;
				COLOR_DECONSTRUCT(samples[i], sr, sg, sb, sa);
				r += sr; g += sg; b += sb; a += sa;
			}
			r /= 4; g /= 4; b /= 4; a /= 4;

			new_img[IDX_2D(x, y, w/2)] = COLOR_RGBA(r, g, b, a);
		}
	}

	return new_img;
}

void gfx_blit(Color* dest, uint dest_w, uint dest_h,
	const Color* src, uint src_w, uint src_h, int x, int y) {
	gfx_blit_ex(dest, dest_w, dest_h, src, src_w, src_h, x, y, 0, 0);
}	

void gfx_blit_ex(Color* dest, uint dest_w, uint dest_h,
	const Color* src, uint src_w, uint src_h, int x, int y,
	int ddx, int ddy) {
	assert(dest);
	assert(src);
	assert(dest_w && dest_h);
	assert(src_w && src_h);

	// Perform clipping on src & dest rectangles 

	int dest_l = x;
	int dest_t = y;
	int dest_r = x + src_w;
	int dest_b = y + src_h;
	
	int src_l = 0;
	int src_t = 0;
	int src_r = src_w;
	int src_b = src_h;

	if(dest_l < 0) {
		src_l += -dest_l;
		dest_l += -dest_l;
	}
	if(dest_r > dest_w) {
		src_r -= dest_r - dest_w;
		dest_r -= dest_r - dest_w;
	}
	if(dest_t < 0) {
		src_t += -dest_t;
		dest_t += -dest_t;
	}
	if(dest_b > dest_h) {
		src_b -= dest_b - dest_h;
		dest_b -= dest_b - dest_h;
	}

	assert(dest_l >= 0 && dest_l <= dest_w);
	assert(dest_r >= 0 && dest_r <= dest_w);
	assert(dest_t >= 0 && dest_t <= dest_h);
	assert(dest_b >= 0 && dest_b <= dest_h);
	assert(src_l >= 0 && src_l <= src_w);
	assert(src_r >= 0 && src_r <= src_w);
	assert(src_t >= 0 && src_t <= src_h);
	assert(src_b >= 0 && src_b <= src_h);
	assert(dest_r - dest_l == src_r - src_l);
	assert(dest_b - dest_t == src_b - src_t);
	
	// Blit

	int w = dest_r - dest_l;
	int h = dest_b - dest_t;

	for(uint dy = 0; dy < h; ++dy) {
		for(uint dx = 0; dx < w; ++dx) {
			size_t d_idx = IDX_2D(dest_l + dx, dest_t + dy, dest_w);
			size_t s_idx = IDX_2D(
				(src_w + src_l + dx - ddx) % src_w, 
				(src_h + src_t + dy - ddy) % src_h, 
				src_w
			);

			dest[d_idx] = endian_swap4(gfx_blend(endian_swap4(dest[d_idx]), 
				endian_swap4(src[s_idx])));
		}
	}
}

void gfx_fill(Color* dest, uint dest_w, uint dest_h,
	Color c, int l, int t, int r, int b) {
	assert(dest);
	assert(dest_w && dest_h);
	assert(l >= 0 && t >= 0);
	assert(r <= dest_w && b <= dest_h);
	assert(l < r && t < b);

	for(uint y = t; y < b; ++y) {
		for(uint x = l; x < r; ++x) {
			size_t idx = IDX_2D(x, y, dest_w);
			dest[idx] = c;
		}
	}
}

extern int stbi_write_tga(const char*, int, int, int, void*);

void gfx_save_tga(const char* filename, Color* img, uint w, uint h) {
	assert(filename);
	assert(img);
	assert(w && h);
	assert(w <= 8192 && h <= 8192);

	stbi_write_tga(filename, w, h, 4, img);
}

