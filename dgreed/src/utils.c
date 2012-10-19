#include "utils.h"
#include "memory.h"
#include "sophist.h"
#include "async.h"

#include <ctype.h>

// For alloca
#ifdef _WIN32
#include <malloc.h>
#endif

#ifdef MACOSX_BUNDLE
#include <CoreFoundation/CFBundle.h>

CFBundleRef main_bundle;
extern const char* g_home_dir;
extern const char* g_storage_dir;

char* path_to_resource(const char* _file) {
	static bool main_bundle_initialized = false;
	if(!main_bundle_initialized) {
		main_bundle_initialized = true;
		main_bundle = CFBundleGetMainBundle();
		if(!main_bundle) { 
			LOG_ERROR("Unable to get reference to Mac OS bundle");
			return 0;
		}	
	}

	char* file = strclone(_file);
	uint length = strlen(file);

	// Separate file and extension by replacing last '.' to 0
	char* extension;
	int cursor = length-1;
	while(cursor >= 0 && file[cursor] != '.') {
		cursor--;
	}
	if(cursor > -1) {
		file[cursor] = '\0';
		extension = &(file[cursor+1]);
	}	
	else {
		extension = NULL;
	}	

	// Separate file and path by replacing last '/' to 0
	char* filename;
	cursor = length-1;
	while(cursor >= 0 && file[cursor] != '/') {
		cursor--;
	}	
	if(cursor > -1) {
		file[cursor] = '\0';
		filename = &(file[cursor+1]);
	}
	else {
		filename = file;
		file = NULL;
	}
    
    CFStringRef filename_cfstring = CFStringCreateWithCString(NULL, filename, kCFStringEncodingASCII);
    CFStringRef extension_cfstring = CFStringCreateWithCString(NULL, extension, kCFStringEncodingASCII);
    CFStringRef file_cfstring = file ? CFStringCreateWithCString(NULL, file, kCFStringEncodingASCII) : NULL;

	CFURLRef resource_url = CFBundleCopyResourceURL(main_bundle, 
                                                    filename_cfstring,
                                                    extension_cfstring,
                                                    file_cfstring
                                                    );
    
    CFRelease(filename_cfstring);
    CFRelease(extension_cfstring);
    if(file)
        CFRelease(file_cfstring);

	if(!resource_url) {
        if(file)
            MEM_FREE(file);
        else
            MEM_FREE(filename);
		return NULL;
	}	

	char* path = MEM_ALLOC(sizeof(char) * 512);
	if(!CFURLGetFileSystemRepresentation(resource_url, true, (UInt8*)path, 512))
		LOG_ERROR("Unable to turn url into path");
    
    CFRelease(resource_url);

	if(file)
            MEM_FREE(file);
        else
            MEM_FREE(filename);
	return path;	
}
#else
char* path_to_resource(const char* _file) {
	return strclone(_file);
}
#endif

/*
-----------------
--- Rectangle ---
-----------------
*/

RectF rectf_null(void) {
	RectF result = {0.0f, 0.0f, 0.0f, 0.0f};
	return result;
}	

RectF rectf(float left, float top, float right, float bottom) {
	RectF result = {left, top, right, bottom};
	return result;
}	

bool rectf_contains_point(const RectF* r, const Vector2* p) {
	assert(r);
	assert(p);

	if(r->left <= p->x && r->right >= p->x)
		if(r->top <= p->y && r->bottom >= p->y)
			return true;
	return false;		
}	

bool rectf_contains_point_rotscale(const RectF* rect, float rot, float scale,
	const Vector2* p) {
	
	Vector2 c = rectf_center(rect);
	Vector2 d = vec2_scale(vec2_rotate(vec2_sub(*p, c), -rot), 1.0f / scale);
	d = vec2_add(c, d);

	return rectf_contains_point(rect, &d);
}

bool rectf_inside_circle(const RectF* rect, const Vector2* p, float r) {
	assert(rect && p);
	assert(r >= 0.0f && r < 1000000.0f);

	// RectF is completely inside circle, if all its points are inside circle
	Vector2 a, b, c, d;
	a = vec2_sub(*p, vec2(rect->left, rect->top));
	b = vec2_sub(*p, vec2(rect->right, rect->top));
	c = vec2_sub(*p, vec2(rect->left, rect->top));
	d = vec2_sub(*p, vec2(rect->right, rect->bottom));

	float r_sq = r*r;
	return vec2_length_sq(a) < r_sq 
		&& vec2_length_sq(b) < r_sq 
		&& vec2_length_sq(c) < r_sq
		&& vec2_length_sq(d) < r_sq;
}

bool rectf_circle_collision(const RectF* rect, const Vector2* p, float r) {
	assert(rect && p);
	assert(r >= 0.0f && r < 1000000.0f);

	float sq_dist = 0.0f;
	if(p->x < rect->left) {
		float d = rect->left - p->x;
		sq_dist += d*d; 
	}
	if(p->x > rect->right) {
		float d = p->x - rect->right;
		sq_dist += d*d;
	}
	if(p->y < rect->top) {
		float d = rect->top - p->y;
		sq_dist += d*d; 
	}
	if(p->y > rect->bottom) {
		float d = p->y - rect->bottom;
		sq_dist += d*d;
	}

	return sq_dist <= r*r;
}

bool _interval_collision(float s1, float e1, float s2, float e2) {
	float t;
	// Assure order
	if(s1 > e1) {
		t = s1; s1 = e1; e1 = t;
	}
	if(s2 > e2) {
		t = s2; s2 = e2; e2 = t;
	}

	// If intervals collide, start of one interval will
	// be inside other interval
	if(s1 <= s2 && s2 <= e1)
		return true;
	if(s2 <= s1 && s1 <= e2)
		return true;
	return false;	
}

bool rectf_rectf_collision(const RectF* r1, const RectF* r2) {
	assert(r1);
	assert(r2);

	// X axis
	if(_interval_collision(r1->left, r1->right, r2->left, r2->right))
		// Y axis
		if(_interval_collision(r1->top, r1->bottom, r2->top, r2->bottom))
			return true;

	return false;
}

float rectf_width(const RectF* r) {
	assert(r);

	return r->right - r->left;
}

float rectf_height(const RectF* r) {
	assert(r);
	
	// CG coordinate system
	return r->bottom - r->top;
}

Vector2 rectf_center(const RectF* r) {
	assert(r);
	return vec2(
		(r->left + r->right) / 2.0f,
		(r->top + r->bottom) / 2.0f
	);	
}

Vector2 rectf_raycast(const RectF* r, const Vector2* start, const Vector2* end) {
	assert(r && start && end);

	Vector2 dir = vec2_normalize(vec2_sub(*end, *start));
	float tmin = 0.0f, tmax = FLT_MAX;

	// Vertical slab
	if(fabsf(dir.x) < 0.0001f) {
		if(start->x < r->left || start->x > r->right)
			return *end;
	}
	else {
		float d = 1.0f / dir.x;
		float t1 = (r->left - start->x) * d;
		float t2 = (r->right - start->x) * d;

		if(t1 > t2) {
			float temp = t1;
			t1 = t2;
			t2 = temp;
		}

		tmin = MAX(tmin, t1);
		tmax = MIN(tmax, t2);

		if(tmin > tmax)
			return *end;
	}

	// Horizontal slab
	if(fabsf(dir.y) < 0.0001f) {
		if(start->y < r->top || start->y > r->bottom)
			return *end;
	}
	else {
		float d = 1.0f / dir.y;
		float t1 = (r->top - start->y) * d;
		float t2 = (r->bottom - start->y) * d;

		if(t1 > t2) {
			float temp = t1;
			t1 = t2;
			t2 = temp;
		}

		tmin = MAX(tmin, t1);
		tmax = MIN(tmax, t2);

		if(tmin > tmax)
			return *end;
	}

	if(tmin*tmin >= vec2_length_sq(vec2_sub(*end, *start)))
		return *end;

	Vector2 hitp = vec2_add(*start, vec2_scale(dir, tmin));

	return hitp;
}

Vector2 rectf_sweep(const RectF* a, const RectF* b, const Vector2* offset) {
	assert(a && b && offset);

	const float eps = 0.001f;

	// Let's make a wrong assumption that all sides of b are shorter or equal to
	// a! For LD23 purposes only.
	
	Vector2 tl = vec2(b->left, b->top);
	Vector2 tr = vec2(b->right, b->top);
	Vector2 bl = vec2(b->left, b->bottom);
	Vector2 br = vec2(b->right, b->bottom);

	Vector2 starts[] = {tl, tr, bl, br};
	bool check[] = {
		offset->x < 0.0f || offset->y < 0.0f,
		offset->x > 0.0f || offset->y < 0.0f,
		offset->x < 0.0f || offset->y > 0.0f,
		offset->x > 0.0f || offset->y > 0.0f
	};
	float min_sq_d = vec2_length_sq(*offset);
	Vector2 min_offset = *offset;
	for(uint i = 0; i < 4; ++i) {
		if(!check[i])
			continue;
		Vector2 end = vec2_add(starts[i], *offset);
		Vector2 nend = rectf_raycast(a, &starts[i], &end);
		Vector2 off = vec2_sub(nend, starts[i]);
		float l = vec2_length_sq(off);
		if(l < min_sq_d) {
			min_sq_d = l;
			min_offset = off;
		}
	}

	// Hack for falling
	if(offset->x == 0.0f && offset->y != 0.0f) {
		if( fabsf(a->left - b->right) < 0.01f ||
			fabsf(a->right - b->left) < 0.01f)  {
			return *offset;
		}
	}

	// Hack for sliding top/bottom
	if(offset->x != 0.0f && offset->y == 0.0f) {
		if( fabsf(a->top - b->bottom) < 0.01f ||
			fabsf(a->bottom - b->top) < 0.01f) {
			return *offset;
		}
	}

	if(min_sq_d > 4.0f * eps * eps)
		return min_offset;
	else
		return vec2(0.0f, 0.0f);
}

uint rectf_cut(const RectF* a, const RectF* b, RectF* out) {
	assert(a && b && out);

	uint cnt = 0;

	if(!rectf_rectf_collision(a, b)) {
		out[0] = *a;
		return 1;
	}

	// Left horizontal slice
	float slice_width = b->left - a->left;
	if(slice_width > 0.0f)
		out[cnt++] = rectf(a->left, a->top, b->left, a->bottom);

	// Middle horizontal slice
	float slice_start = MAX(a->left, b->left);
	float slice_end = MIN(a->right, b->right);

	// Top vertical slice
	float slice_height = b->top - a->top;
	if(slice_height > 0.0f)
		out[cnt++] = rectf(slice_start, a->top, slice_end, b->top);
	
	// Bottom vertical slice
	slice_height = a->bottom - b->bottom;
	if(slice_height > 0.0f)
		out[cnt++] = rectf(slice_start, b->bottom, slice_end, a->bottom);

	// Right horizontal slice
	slice_width = a->right - b->right;
	if(slice_width > 0.0f)
		out[cnt++] = rectf(b->right, a->top, a->right, a->bottom);

	return cnt;
}

RectF rectf_bbox(const RectF* a, const RectF* b) {
	RectF bbox = *a;

	bbox.left = MIN(bbox.left, b->left);
	bbox.right = MAX(bbox.right, b->right);
	bbox.top = MIN(bbox.top, b->top);
	bbox.bottom = MAX(bbox.bottom, b->bottom);

	return bbox;
}

/*
---------------
--- Triangle --
---------------
*/

bool tri_contains_point(const Triangle* tri, const Vector2* p) {
	assert(tri && p);

	Vector2 a = vec2_sub(tri->p3, tri->p2);
	Vector2 b = vec2_sub(tri->p1, tri->p3);
	Vector2 c = vec2_sub(tri->p2, tri->p1);
	Vector2 ap = vec2_sub(*p, tri->p1);
	Vector2 bp = vec2_sub(*p, tri->p2);
	Vector2 cp = vec2_sub(*p, tri->p3);

	float abp = a.x*bp.y - a.y*bp.x;
	float cap = c.x*ap.y - c.y*ap.x;
	float bcp = b.x*cp.y - b.y*cp.x;

	return (abp >= 0.0f) && (cap >= 0.0f) && (bcp >= 0.0f);
}

bool tri_rectf_collision(const Triangle* tri, const RectF* rect) {
	assert(tri);
	assert(rect);

	if(rectf_contains_point(rect, &(tri->p1)) 
		|| rectf_contains_point(rect, &(tri->p2))
		|| rectf_contains_point(rect, &(tri->p3)))
		return true;

	Vector2 p1 = vec2(rect->left, rect->top);
	Vector2 p2 = vec2(rect->left, rect->bottom);
	Vector2 p3 = vec2(rect->right, rect->top);
	Vector2 p4 = vec2(rect->right, rect->bottom);

	if(tri_contains_point(tri, &p1)
		|| tri_contains_point(tri, &p2)
		|| tri_contains_point(tri, &p3)
		|| tri_contains_point(tri, &p4))
		return true;

	return false;
}	

/*
--------------------
--- Line segment ---
--------------------
*/

Segment segment(Vector2 p1, Vector2 p2) {
	Segment new = {p1, p2};
	return new; 
}

float segment_length(Segment s) {
	return vec2_length(vec2_sub(s.p1, s.p2));
}

float segment_point_dist(Segment s, Vector2 p) {
	float dx = s.p2.x - s.p1.x;
	float dy = s.p2.y - s.p1.y;
	float px = p.x - s.p1.x;
	float py = p.y - s.p1.y;
	float d = dx*py - dy*px;
	float d_sgn = d < 0.0f ? 1.0f : -1.0f;

	float dot1 = vec2_dot(vec2_sub(s.p2, s.p1), vec2_sub(p, s.p2));
	float dot2 = vec2_dot(vec2_sub(s.p1, s.p2), vec2_sub(p, s.p1));

	if(dot2 > 0.0f)
		return vec2_length(vec2_sub(s.p1, p)) * d_sgn;
	if(dot1 > 0.0f)
		return vec2_length(vec2_sub(s.p2, p)) * d_sgn;
	return -d / sqrt(dx*dx + dy*dy);	
}

bool segment_intersect(Segment s1, Segment s2, Vector2* p) {
	const float epsilon = 0.001f;

	float a1 = s1.p2.y - s1.p1.y;
	float b1 = s1.p1.x - s1.p2.x;
	float c1 = a1*s1.p1.x + b1*s1.p1.y;
	float a2 = s2.p2.y - s2.p1.y;
	float b2 = s2.p1.x - s2.p2.x;
	float c2 = a2*s2.p1.x + b2*s2.p1.y;

	float det = a1*b2 - a2*b1;
	if(fabsf(det) < epsilon)
		return false;

	float x = (b2*c1 - b1*c2) / det;
	float y = (a1*c2 - a2*c1) / det;
	if(MIN(s1.p1.x, s1.p2.x) <= (x+epsilon) &&
		MAX(s1.p1.x, s1.p2.x) >= (x-epsilon) &&
		MIN(s2.p1.x, s2.p2.x) <= (x+epsilon) &&
		MAX(s2.p1.x, s2.p2.x) >= (x-epsilon)) {
			if(MIN(s1.p1.y, s1.p2.y) <= (y+epsilon) &&
				MAX(s1.p1.y, s1.p2.y) >= (y-epsilon) &&
				MIN(s2.p1.y, s2.p2.y) <= (y+epsilon) &&
				MAX(s2.p1.y, s2.p2.y) >= (y-epsilon)) {
					if(p)
						*p = vec2(x, y);
					return true;		
			}	
	}	
	return false;
}

static bool _in_interval(float start, float end, float x) {
	if(start <= x && x <= end)
		return true;
	if(end <= x && x <= start)
		return true;
	return false;
}

Vector2 circle_raycast(const Vector2* center, float r,
		const Vector2* start, const Vector2* end) {

	assert(center && start && end);
	assert(r > 0.0f);

	Vector2 s = vec2_sub(*start, *center);
	Vector2 n = vec2_normalize(vec2_sub(*end, *start));

	float b = vec2_dot(s, n);
	float c = vec2_length_sq(s) - r*r;

	if(b > 0.0f && c > 0.0f)
		return *end;

	float d_sqr = b*b - c;

	if(d_sqr < 0.0f)
		return *end;

	float t = -b - sqrtf(d_sqr);
	if(t < 0.0f) 
		t = 0.0f;

	Vector2 hitp = vec2_add(*start, vec2_scale(n, t));
	if(_in_interval(start->x, end->x, hitp.x) &&
		_in_interval(start->y, end->y, hitp.y))
		return hitp;

	return *end;
}

float circle_circle_test(
		Vector2 a_center, float a_radius, Vector2 a_offset,
		Vector2 b_center, float b_radius, Vector2 b_offset) {
	assert(a_radius > 0.0f && b_radius > 0.0f);

	Vector2 ab = vec2_sub(b_center, a_center);
	Vector2 rel = vec2_sub(b_offset, a_offset);
	float r = a_radius + b_radius;
	float c = vec2_dot(ab, ab) - r*r;
	if(c < 0.0f)		// Already overalapping at t=0
		return 0.0f;

	float a = vec2_dot(rel, rel);
	if(a < 0.00001f)	// No relative movement 
		return -1.0f;
	float b = vec2_dot(rel, ab);
	if(b >= 0.0f)		// Not moving towards
		return -1.0f;
	float d = b*b - a*c;
	if(d < 0.0f)		// No intersection
		return -1.0f;

	float t = (-b - sqrtf(d)) / a;
	if(t <= 1.0f)
		return t;
	else
		return -1.0f;
}


/*
--------------
--- Colors ---
--------------
*/

#define F_TO_B(f) ((byte)(f * 255.0f))

Color hsv_to_rgb(ColorHSV hsv) {
	float f, p, q, t;
	byte i, bp, bq, bt, ba, bv;

	bv = F_TO_B(hsv.v);
	ba = F_TO_B(hsv.a);
	if(hsv.s == 0.0f) 
		return COLOR_RGBA(bv, bv, bv, ba);

	hsv.h *= 6.0f;
	i = (byte)floor(hsv.h);
	f = hsv.h - i;

	p = hsv.v * (1.0f - hsv.s);
	q = hsv.v * (1.0f - (hsv.s * f));
	t = hsv.v * (1.0f - (hsv.s * (1.0f - f)));

	bp = F_TO_B(p);
	bq = F_TO_B(q);
	bt = F_TO_B(t);

	switch(i) {
		case 6:
		case 0:
			return COLOR_RGBA(bv, bt, bp, ba);
		case 1:
			return COLOR_RGBA(bq, bv, bp, ba);
		case 2:
			return COLOR_RGBA(bp, bv, bt, ba);
		case 3:
			return COLOR_RGBA(bp, bq, bv, ba);
		case 4:
			return COLOR_RGBA(bt, bp, bv, ba);
		case 5:
			return COLOR_RGBA(bv, bp, bq, ba);
		default:
			assert(false);
			return COLOR_TRANSPARENT;
	}
}	

ColorHSV rgb_to_hsv(Color rgb) {
	ColorHSV hsv;
	hsv.a = 0.0f;
	float fr, fg, fb, fa;
	fr = (float)(rgb & 0xFF) / 255.0f;
	fg = (float)((rgb >> 8) & 0xFF) / 255.0f;
	fb = (float)((rgb >> 16) & 0xFF) / 255.0f;
	fa = (float)((rgb >> 24) & 0xFF) / 255.0f;

	float min = MIN(fr, MIN(fg, fb));
	float max = MAX(fr, MAX(fg, fb));
	hsv.v = max;
	float delta = max - min;

	if(max != 0.0f) {
		hsv.s = delta / max;
	}
	else {
		hsv.s = 0.0f; hsv.h = 0.0f;
		return hsv;
	}	
	if(fr == max) 
		hsv.h = (fg - fb) / delta;
	else if (fg == max)
		hsv.h = 2.0f + (fb - fr) / delta;
	else
		hsv.h = 4.0f + (fr - fg) / delta;
	hsv.h /= 6.0f;	
	if(hsv.h < 0.0f)
		hsv.h += 1.0f;
	hsv.a = fa;	
	return hsv;	
}	

Color color_lerp(Color c1, Color c2, float t) {
	if(t <= 0.0f)
		return c1;
	if(t >= 1.0f)
		return c2;

	byte r1, g1, b1, a1;
	byte r2, g2, b2, a2;
	byte r, g, b, a;
	uint bt = (uint)(t * 255.0f);

	r1 = c1 & 0xFF; c1 >>= 8;
	g1 = c1 & 0xFF; c1 >>= 8;
	b1 = c1 & 0xFF; c1 >>= 8;
	a1 = c1 & 0xFF;
	r2 = c2 & 0xFF; c2 >>= 8;
	g2 = c2 & 0xFF; c2 >>= 8;
	b2 = c2 & 0xFF; c2 >>= 8;
	a2 = c2 & 0xFF;

	r = r1 + (((r2 - r1) * bt) >> 8);
	g = g1 + (((g2 - g1) * bt) >> 8);
	b = b1 + (((b2 - b1) * bt) >> 8);
	a = a1 + (((a2 - a1) * bt) >> 8);
	return COLOR_RGBA(r, g, b, a);
}	

/*
----------------------
--- Random numbers ---
----------------------
*/

// MT19937 implementation

#define MT19937_N 624
#define MT19937_M 397

typedef struct {
	uint mt_state[MT19937_N];
	uint mt_index;
} RndState;

static RndState global_rnd;
static RndContext global_rndctx = &global_rnd; 

static void _mt_init(RndState* state, uint seed) {
	state->mt_state[0] = seed;
	for(uint i = 1; i < MT19937_N; ++i) {
		uint t = (state->mt_state[i-1] ^ (state->mt_state[i-1] >> 30));
		state->mt_state[i] = 1812433253UL * (t + i);
	}
	state->mt_index = MT19937_N;
}

static void _mt_update(RndState* state) {
	int kk;
	uint y;

	uint* mt_state = state->mt_state;

	const uint upper = 0x80000000UL;
	const uint lower = 0x7fffffffUL;
	const uint matrix = 0x9908b0dfUL;

	for(kk = 0; kk < MT19937_N - MT19937_M; ++kk) {
		y = (mt_state[kk] & upper) | (mt_state[kk+1] & lower);
		mt_state[kk] = mt_state[kk + MT19937_M] ^ (y >> 1); 
		mt_state[kk] ^= (y & 1) ? 0 : matrix;
	}

	for(; kk < MT19937_N - 1; ++kk) {
		y = (mt_state[kk] & upper) | (mt_state[kk+1] & lower);
		mt_state[kk] = mt_state[kk + (MT19937_M - MT19937_N)] ^ (y >> 1);
		mt_state[kk] ^= (y & 1) ? 0 : matrix;
	}

	y = (mt_state[kk] & upper) | (mt_state[0] & lower);
	mt_state[kk] = mt_state[kk + (MT19937_M - MT19937_N)] ^ (y >> 1);
	mt_state[kk] ^= (y & 1) ? 0 : matrix;

	state->mt_index = 0;
}

static uint _mt_uint(RndState* state) {
	if(state->mt_index >= MT19937_N)
		_mt_update(state);
	
	uint result = state->mt_state[state->mt_index++];

	result ^= (result >> 11);
	result ^= (result << 7) & 0x9d2c5680UL;
	result ^= (result << 15) & 0xefc60000UL;
	result ^= (result >> 18);

	return result;
}

// Public interface

void rand_init(uint seed) {
	rand_init_ex(&global_rndctx, seed);
}

uint rand_uint(void) {
	return rand_uint_ex(&global_rndctx);
}

int rand_int(int min, int max) {
	return rand_int_ex(&global_rndctx, min, max);
}

float rand_float(void) {
	return rand_float_ex(&global_rndctx);
}

float rand_float_range(float min, float max) {
	return rand_float_range_ex(&global_rndctx, min, max);
}

void rand_init_ex(RndContext* ctx, uint seed) {
	*ctx = malloc(sizeof(RndState));
	rand_seed_ex(ctx, seed);
}

void rand_seed_ex(RndContext* ctx, uint seed) {
	RndState* st = *ctx;
	_mt_init(st, seed);
}

void rand_free_ex(RndContext* ctx) {
	free(*ctx);
}

uint rand_uint_ex(RndContext* ctx) {
	RndState* st = *ctx;
	return _mt_uint(st);
}

int rand_int_ex(RndContext* ctx, int min, int max) {
	int range;

	assert(min < max);

	range = max - min;
	return (rand_uint_ex(ctx)%range) + min;
}

float rand_float_ex(RndContext* ctx) {
	return (float)(rand_uint_ex(ctx)>>1) / (float)(MAX_UINT32>>1);
}

float rand_float_range_ex(RndContext* ctx, float min, float max) {
	return min + rand_float_ex(ctx) * (max - min);
}

/*
---------------
--- Logging ---
---------------
*/

FILE* log_file = NULL;
uint log_level = LOG_LEVEL_INFO;
CriticalSection log_cs;

void LOG_ERROR(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_send(LOG_LEVEL_ERROR, format, args);
	va_end(args);

	log_close();
	exit(0);
}
void LOG_WARNING(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_send(LOG_LEVEL_WARNING, format, args);
	va_end(args);
}
void LOG_INFO(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_send(LOG_LEVEL_INFO, format, args);
	va_end(args);
}	
static const char* log_level_to_cstr(uint log_level) {
	switch(log_level) {
		case LOG_LEVEL_ERROR:
			return "ERROR";
		case LOG_LEVEL_WARNING:
			return "WARNING";
		case LOG_LEVEL_INFO:
			return "INFO";
		default:
			return "???";
	}
}

bool log_init(const char* log_path, uint level) {
	// Log to stderr on iOS
#ifdef TARGET_IOS
	log_file = stderr;
#else	
	#ifdef MACOSX_BUNDLE
		char bundle_log_path[512];
		sprintf(bundle_log_path, "%s/Library/Logs/%s", g_home_dir, log_path);
		log_file = fopen(bundle_log_path, "w");
	#else
		if(log_path)
			log_file = fopen(log_path, "w");
		else
			log_file = stderr;
	#endif
#endif
	
	if(log_file == NULL)
			return false;

	log_level = level;	

	log_cs = async_make_cs();

	LOG_INFO("Log initialized");
	return true;
}

void log_close(void) {
	assert(log_file);

	LOG_INFO("Log closed");
	if(log_file != stderr)
		fclose(log_file);
	log_file = stderr;
}

/* TODO: Display time */
void log_send(uint level, const char* format, va_list args) {
	char msg_buffer[LOG_MSG_BUFFER_SIZE];

	assert(log_file);

	if(level > log_level)
			return;

	vsnprintf(msg_buffer, LOG_MSG_BUFFER_SIZE, format, args);
	
	async_enter_cs(log_cs);
	fprintf(log_file, "%s: %s\n", log_level_to_cstr(log_level), msg_buffer);
	async_leave_cs(log_cs);
}

/*
---------------------------
--- Parameters handling ---
---------------------------
*/

uint largc = 0;
const char** largv = NULL;

void params_init(uint argc, const char** argv) {
	largc = argc;
	largv = argv;
}

uint params_count(void) {
	return largc - 1;
}

const char* params_get(uint n) {
	assert(largv);
	assert(n < largc - 1);
	return largv[n + 1];
}

uint params_find(const char* param) {
	uint i;

	for(i = 1; i < largc; ++i) {
		if(strcmp(largv[i], param) == 0) return i-1;
	}

	return ~0;
}

/*
---------------
--- File IO ---
---------------
*/

bool fs_devmode = false; // If this is true, also look for files in 'prefix + filename'
const char* fs_devmode_prefix = "../../bin/";

#ifdef MACOSX_BUNDLE
static FILE* _storage_fopen(const char* name, const char* mode) {
	char storage_path[256];
	const char* file = path_get_file(name);
    if(file) {
        assert(strlen(g_storage_dir) + strlen(file) < 254);
        sprintf(storage_path, "%s/%s", g_storage_dir, file);
        return fopen(storage_path, mode);
    }
    else {
        return NULL;
    }
}

static const char* _storage_fpath(const char* name) {
	char storage_path[256];
	const char* file = path_get_file(name);
    if(file) {
        assert(strlen(g_storage_dir) + strlen(file) < 254);
        sprintf(storage_path, "%s/%s", g_storage_dir, file);
        return strclone(storage_path);
    }
    else {
        return NULL;
    }
}
#endif

bool file_exists(const char* name) {
	assert(name);

#ifdef MACOSX_BUNDLE
	
	FILE* file = _storage_fopen(name, "rb");
	if(file != NULL) {
		fclose(file);
		return true;
	}

	char* path = path_to_resource(name);
    if(path) {
        file = fopen(path, "rb");
        MEM_FREE(path);
    }
    else {
        file = NULL;
    }
#else	
	FILE* file = fopen(name, "rb");
#endif

	if(file != NULL) {
		fclose(file);
		return true;
	}

	if(fs_devmode) {
		char dev_path[256];
		strcpy(dev_path, fs_devmode_prefix);
		strcat(dev_path, name);
		file = fopen(dev_path, "rb");
		if(file != NULL) {
			fclose(file);
			return true;
		}
	}

	return false;
}

void file_move(const char* old_name, const char* new_name) {
	assert(old_name && new_name);

	const char* path = NULL;

#ifdef MACOSX_BUNDLE
	FILE* file = _storage_fopen(old_name, "rb");
	if(file != NULL) {
		fclose(file);
		path = _storage_fpath(old_name);
	}

	if(!path) {
		const char* res_path = path_to_resource(old_name);
		if(res_path)
			path = res_path;
	}
#endif

	if(path == NULL)
		path = strclone(old_name);
	assert(path);

	const char* folder = path_get_folder(path);
	assert(folder);

	char* new_path = alloca(strlen(folder) + strlen(new_name) + 8);
	sprintf(new_path, "%s%s", folder, new_name);

	rename(path, new_path);

	MEM_FREE(path);
}

void file_remove(const char* name) {
	assert(name);

	const char* path = NULL;

#ifdef MACOSX_BUNDLE
	FILE* file = _storage_fopen(name, "rb");
	if(file != NULL) {
		fclose(file);
		path = _storage_fpath(name);
	}

	if(!path) {
		const char* res_path = path_to_resource(name);
		if(res_path)
			path = res_path;
	}
#endif

	if(path == NULL)
		path = strclone(name);
	assert(path);

	remove(path);

	MEM_FREE(path);
}

FileHandle file_open(const char* name) {
	assert(name);

#ifdef MACOSX_BUNDLE
	char* path = path_to_resource(name);
    FILE* f = NULL;
	if(path) 
		f = fopen(path, "rb");

	if(f == NULL) 
		f = _storage_fopen(name, "rb");
	if(path)
		MEM_FREE(path);

#else
	FILE* f = fopen(name, "rb");
#endif

	if(f == NULL) {
		if(fs_devmode) {
			char dev_path[256];
			strcpy(dev_path, fs_devmode_prefix);
			strcat(dev_path, name);
			f = fopen(dev_path, "rb");
			if(f != NULL) 
				return (FileHandle)f;
		}
	
		LOG_ERROR("Unable to open file %s", name);
		return 0;
	}

	return (FileHandle)f;
}

void file_close(FileHandle f) {
	FILE* file = (FILE*)f;

	fclose(file);
}

uint file_size(FileHandle f) {
	FILE* file = (FILE*)f;
	uint pos, size;

	assert(file);

	pos = ftell(file);
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, pos, SEEK_SET);

	return size;
}

void file_seek(FileHandle f, uint pos) {
	FILE* file = (FILE*)f;

	assert(file);

	fseek(file, pos, SEEK_SET);
}

byte file_read_byte(FileHandle f) {
	FILE* file = (FILE*)f;
	byte result;
	uint read;

	assert(file);

	read = fread(&result, 1, 1, file);
	if(read != 1)
		LOG_ERROR("File reading error. Unexpected EOF?");

	return result;
}

uint16 file_read_uint16(FileHandle f) {
	FILE* file = (FILE*)f;
	uint16 result;
	uint read;

	assert(file);

	read = fread(&result, 1, 2, file);
	if(read != 2)
		LOG_ERROR("File reading error. Unexpected EOF?");

	result = endian_swap2(result);
	return result;
}

uint32 file_read_uint32(FileHandle f) {
	FILE* file = (FILE*)f;
	uint32 result;
	uint read;

	assert(file);

	read = fread(&result, 1, 4, file);
	if(read != 4)
		LOG_ERROR("File reading error. Unexpected EOF?");

	result = endian_swap4(result);
	return result;
}

float file_read_float(FileHandle f) {
	uint32 r = file_read_uint32(f);
	void* rp = &r;
	return *((float*)rp);
}

void file_read(FileHandle f, void* dest, uint size) {
	FILE* file = (FILE*)f;
	uint read;
	
	assert(file);

	read = fread(dest, 1, size, file);
	if(read != size)
		LOG_ERROR("File reading error. Unexpected EOF?");

}		

FileHandle file_create(const char* name) {
	assert(name);

#ifdef MACOSX_BUNDLE
	FILE* file = _storage_fopen(name, "wb");
#else
	FILE* file = fopen(name, "wb");
#endif

	if(file == NULL) {
		LOG_ERROR("Unable to open file %s for writing", name);
		return 0;
	}

	return (FileHandle)file;
}

void file_write_byte(FileHandle f, byte data) {
	FILE* file = (FILE*)f;

	assert(file);

	if(fwrite((void*)&data, 1, 1, file) != 1)
		LOG_ERROR("File writing error");
}

void file_write_uint16(FileHandle f, uint16 data) {
	FILE* file = (FILE*)f;

	assert(file);

	data = endian_swap2(data);
	if(fwrite((void*)&data, 1, 2, file) != 2)
		LOG_ERROR("File writing error");
}		

void file_write_uint32(FileHandle f, uint32 data) {
	FILE* file = (FILE*)f;

	assert(file);

	data = endian_swap4(data);
	if(fwrite((void*)&data, 1, 4, file) != 4)
		LOG_ERROR("File writing error");
}

void file_write_float(FileHandle f, float data) {
	void* pd = &data;
	file_write_uint32(f, *((uint32*)pd));
}	

void file_write(FileHandle f, const void* data, uint size) {
	FILE* file = (FILE*)f;

	assert(file);

	if(fwrite(data, 1, size, file) != size)
		LOG_ERROR("File writing error");
}

void txtfile_write(const char* name, const char* text) {
	assert(name);
	assert(text);
	
#ifdef MACOSX_BUNDLE
	FILE* file = _storage_fopen(name, "w");
#else
	FILE* file = fopen(name, "w");
#endif

	if(!file)
		LOG_ERROR("Unable to open file %s for writing", name);

	uint size = strlen(text);
	fwrite(text, 1, size, file);
	fclose(file);
}	

char* txtfile_read(const char* name) {
	FileHandle file = file_open(name);
	uint size = file_size(file);

	char* out = MEM_ALLOC(size+1);
	out[size] = '\0';
	file_read(file, out, size); 
	file_close(file);
	return out;
}	

bool _is_delim(char c) {
	return c == '/' || c == '\\';
}	

char* path_get_folder(const char* path) {
	assert(path);
	size_t len = strlen(path);
	if(len == 0)
		return NULL;

	char last = path[len-1];

	// If path ends with delimiter or path is a dot - 
	// filename is already removed, complain
	if(_is_delim(last) || (len == 1 && last == '.'))
		return NULL;
	
	// Calculate stripped path length
	int idx = len-1;
	while(idx >= 0 && !_is_delim(path[idx])) {
		idx--;
	}	
	size_t new_len = idx+2;
	
	// If all path was just filename, return current dir
	if(new_len < 3)
		return strclone("./");

	char* result = (char*)MEM_ALLOC(new_len);
	strncpy(result, path, new_len-1);
	result[new_len-1] = '\0';
	return result;
}

const char* path_get_file(const char* path) {
	assert(path);
	int len = strlen(path);
	int i = len;
	while(i > 0 && !_is_delim(path[--i]));
	if(i == 0)
		return path;
	if(_is_delim(path[i]))
	   return i+1 < len ? &path[i+1] : NULL;
	else
	   return NULL;
}

bool _validate_extension(const char* ext) {
	assert(ext);
	size_t len = strlen(ext);

	if(len == 0)
		return true;

	if(ext[0] == '.')
		return false;

	for(uint i = 0; i < len; ++i) {
		if(!isalnum(ext[i]))
			return false;
	}
	return true;
}	

char* path_change_ext(const char* path, const char* ext) {
	assert(path);
	assert(ext);

	size_t path_len = strlen(path);
	if(path_len == 0)
		return NULL;
	char path_last = path[path_len-1];

	// Avoid invalid path
	if(_is_delim(path_last) || (path_len == 1 && path_last == '.'))
		return NULL;

	// Avoid invalid extension
	if(_validate_extension(ext) == false)
		return NULL;

	// Find where current extension begins (end of string if no extension)
	int ext_start = path_len-1;
	while(ext_start >= 0 && !_is_delim(path[ext_start]) 
			&& path[ext_start] != '.') {
		ext_start--;
	}	
	assert(ext_start >= 0);
	if(ext_start == 0 || path[ext_start] != '.' || 
			(path[ext_start] == '.' && _is_delim(path[ext_start-1])))
		// No extension found
		ext_start = path_len-1;
	else
		ext_start--;

	size_t ext_len = strlen(ext);
	size_t new_len = ext_start+1 + (ext_len > 0 ? ext_len + 1 : 0);
	char* result = (char*)MEM_ALLOC(new_len+1);
	strncpy(result, path, ext_start+2);
	if(ext_len == 0) {
		result[ext_start+1] = '\0';
		return result;
	}
	result[ext_start+1] = '.';
	strncpy(&result[ext_start+2], ext, ext_len);
	result[new_len] = '\0';
	return result;
}

/*
------------
--- Misc ---
------------
*/

uint16 endian_swap2(uint16 in) {
#if SOPHIST_endian == SOPHIST_big_endian
	return ((in << 8) & 0xFF00) | ((in >> 8) & 0xFF);
#else
	return in;
#endif	
}

uint32 endian_swap4(uint32 in) {
#if SOPHIST_endian == SOPHIST_big_endian
		return ((in << 24) & 0xFF000000) | ((in << 8) & 0x00FF0000) | \
			((in >> 8) & 0x0000FF00) | ((in >> 24) & 0x000000FF);
#else			
		return in;
#endif		
}

char* strclone_untracked(const char* str) {
	assert(str);

	uint len = strlen(str);
	char* clone = MEM_ALLOC(len+1);
	strcpy(clone, str);
	return clone;
}

#ifdef TRACK_MEMORY
char* strclone_tracked(const char* str, const char* file, int line) {
	assert(str);

	uint len = strlen(str);
	char* clone = mem_alloc(len+1, file, line);
	strcpy(clone, str);
	return clone;
}
#endif

float lerp(float a, float b, float t) {
	 return a + (b - a) * t;
}	 

float smoothstep(float a, float b, float t) {
	return lerp(a, b, t * t * (3.0f - 2.0f * t));
}	

float normalize(float val, float min, float max) {
	assert(min < max);

	return (val - min) / (max - min);
}

float clamp(float min, float max, float val) {
	assert(min < max);
	return MAX(min, MIN(max, val));
}	

bool feql(float a, float b) {
	return fabsf(a - b) < 0.00001f;
}

bool is_pow2(uint n) {
	return n && !(n & (n - 1));
}

uint next_pow2(uint n) {
    uint p = 1;
    while(n > p)
        p *= 2;
    return p;
}

// Knuth-Morris-Pratt with stack allocated partial match table
int strfind(const char* needle, const char* haystack) {
	assert(needle && haystack);

	// Special cases when needle is shorter than 2 symbols
	int needle_len = strlen(needle);
	if(needle_len < 1)
		return 0;
	if(needle_len == 1) {
		for(uint i = 0; haystack[i] != '\0'; ++i)
			if(haystack[i] == needle[0])
				return i;
		return -1;
	}
	
	// Special cases when haystack is not longer than needle
	int haystack_len = strlen(haystack);
	if(haystack_len < needle_len)
		return -1;
	if(haystack_len == needle_len)
		return strcmp(needle, haystack) == 0 ? 0 : -1;

	// Build partial match table
	int* t = alloca(needle_len * sizeof(int));
	t[0] = -1; t[1] = 0;
	for(int i = 2, j = 0; i < needle_len; ++i) {
		if(needle[i-1] == needle[j]) {
			j++;
			t[i] = j;
		}
		else if(j > 0) {
			j = t[j];
			i--;
		}
		else {
			t[i] = 0;
		}
	}
	
	// Search
	int m = 0, i = 0;
	while(m + i < haystack_len) {
		if(needle[i] == haystack[m + i]) {
			if(i == needle_len - 1)
				return m;
			++i;
		}
		else {
			m = m + i - t[i];
			i = MAX(0, t[i]);
		}
	}

	return -1;
}

/*
-------------------
--- Compression ---
-------------------
*/

#define WINDOW_BITS 11
#define LENGTH_BITS 5
#define MIN_MATCH 3

#define WINDOW_SIZE (1<<WINDOW_BITS)
#define MAX_MATCH (MIN_MATCH + (1 << LENGTH_BITS) - 1)

void* lz_compress(void* input, uint input_size, uint* output_size)
{
	byte* data = (byte*) input;
	byte* compressed;
	uint read_ptr, write_ptr, flag_ptr, window_ptr, bit;
	uint i, match_len, best_match, best_match_ptr;
	byte flag;
	uint16 pair;
	
	assert(input);
	assert(input_size);
	assert(output_size);
	assert(WINDOW_BITS + LENGTH_BITS == 16);

	/* Worst case - 1 additional bit per byte + 4 bytes for original size */
	compressed = MEM_ALLOC(input_size * 9 / 8 + 4);

	/* Write original size */
	compressed[0] = input_size & 0x000000FF;
	compressed[1] = (input_size & 0x0000FF00) >> 8;
	compressed[2] = (input_size & 0x00FF0000) >> 16;
	compressed[3] = (input_size & 0xFF000000) >> 24;

	read_ptr = 0; write_ptr = 4;

	while(read_ptr < input_size) {
		flag_ptr = write_ptr++;	
		flag = 0;

		for(bit = 0; bit < 8 && read_ptr < input_size; ++bit) {
			window_ptr = read_ptr > WINDOW_SIZE ? read_ptr - WINDOW_SIZE : 0;

			best_match = best_match_ptr = 0;
			for(i = 0; i < WINDOW_SIZE; ++i) {
				if(window_ptr + i == read_ptr)
					break;

				match_len = 0;
				while(read_ptr + match_len < input_size) {
					if(data[read_ptr+match_len] != data[window_ptr+i+match_len])
						break;
					if(match_len == MAX_MATCH)
						break;
					match_len++;	
				}

				if(match_len > best_match) {
					best_match = match_len;
					best_match_ptr = i;
				}	
			}

			if(best_match >= MIN_MATCH) {
				flag |= 1 << bit;
				pair = best_match_ptr | ((best_match-MIN_MATCH) << (16 - LENGTH_BITS));
				compressed[write_ptr++] = pair & 0xFF;
				compressed[write_ptr++] = pair >> 8;
				read_ptr += best_match;
			}
			else {
				compressed[write_ptr++] = data[read_ptr++];
			}
		}
		compressed[flag_ptr] = flag;
	}	

	*output_size = write_ptr;
	return compressed;
}

void* lz_decompress(void* input, uint input_size, uint* output_size) {
	byte flag;
	uint read_ptr, write_ptr, window_ptr, bit;
	uint offset, length, i;
	byte* data = (byte*)input;
	byte* decompressed;
	uint16 pair;

	assert(input);
	assert(input_size);
	assert(output_size);
	assert(WINDOW_BITS + LENGTH_BITS == 16);

	*output_size = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	read_ptr = 4;
	write_ptr = 0;

	decompressed = MEM_ALLOC(*output_size);

	while(write_ptr < *output_size) {
		flag = data[read_ptr++];
		for(bit = 0; bit < 8 && write_ptr < *output_size; ++bit) {
			if(!(flag & (1 << bit))) {
				decompressed[write_ptr++] = data[read_ptr++];
				continue;
			}

			window_ptr = write_ptr > WINDOW_SIZE ? write_ptr - WINDOW_SIZE : 0;		
			pair = data[read_ptr++];
			pair |= data[read_ptr++] << 8;
			offset = pair & (WINDOW_SIZE-1);
			length = pair >> (16 - LENGTH_BITS);
			length += MIN_MATCH;

			for(i = 0; i < length; ++i) 
				decompressed[write_ptr+i] = decompressed[window_ptr+offset+i];

			write_ptr += length;
		}
	}	

	return decompressed;
}

/*
--------------
--- Base64 ---
--------------
*/

static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static byte base64_inv[256] = {0};
static bool base64_initialized = false;

static void _base64_init(void) {
	for(uint i = 0; i < 64; ++i)
		base64_inv[(uint)base64_chars[i]] = i;
	base64_initialized = true;	
}

// Encodes 3 bytes to 4 chars
static void _base64_encode_chunk(const byte* in, char* out) {
	out[0] = base64_chars[in[0] & ((1<<6)-1)];
	out[1] = base64_chars[(in[0] >> 6) | ((in[1] & ((1<<4)-1)) << 2)];
	out[2] = base64_chars[(in[1] >> 4) | ((in[2] & ((1<<2)-1)) << 4)];
	out[3] = base64_chars[in[2] >> 2];
}

// Decodes 4 chars to 3 bytes
static void _base64_decode_chunk(const char* in, byte* out, uint padding) {
	byte* inp = (byte*)in;
	switch(padding) {
		default:
			out[2] = base64_inv[inp[2]] >> 4 | base64_inv[inp[3]] << 2;
		case 1:
			out[1] = base64_inv[inp[1]] >> 2 | base64_inv[inp[2]] << 4;
		case 2:
			out[0] = base64_inv[inp[0]] | base64_inv[inp[1]] << 6;
	}		
}

char* base64_encode(const void* input, uint input_size, uint* output_size) {
	assert(input && input_size);
	assert(output_size);

	uint padding = input_size % 3;
	*output_size = (input_size / 3) * 4 + (padding ? 4 : 0);
	char* output = MEM_ALLOC(*output_size);
	const byte* inp = (byte*)input;

	for(uint i = 0, o = 0; i < input_size; i += 3, o += 4) {
		uint left = input_size - i;
		byte chunk[3] = {0};

		for(uint j = 0; j < MIN(3, left); ++j)
			chunk[j] = inp[i + j];

		_base64_encode_chunk(chunk, &output[o]);

		if(left < 3) {
			output[o + 3] = '=';
			if(left < 2)
				output[o + 2] = '=';
		}		
	}

	return output;
}

void* base64_decode(const char* input, uint input_size, uint* output_size) {
	assert(input && input_size);
	assert(output_size);

	if(!base64_initialized)
		_base64_init();

	uint padding = 0, i = input_size;
	while(i && input[--i] == '=')
		padding++;

	*output_size = input_size / 4 * 3 - padding;
	byte* output = MEM_ALLOC(*output_size);

	size_t isize = input_size - padding;
	for(uint i = 0, o = 0; i < isize; i += 4, o += 3)
		_base64_decode_chunk(
			&input[i], &output[o], 
			(isize - i) < 4 ? 4 - (isize - i) : 0
		);

	return output;
}

/*
---------------
--- Hashing ---
---------------
*/

uint hash_murmur(const void* data, uint len, uint seed) {
	const uint m = 0x5bd1e995;
	const int r = 24;
	uint h = seed ^ len;
	const byte* bdata = (const byte*)data;

	while(len >= 4) {

#if SOPHIST_endian == SOPHIST_big_endian
		uint k = bdata[0];
		k |= bdata[1] << 8;
		k |= bdata[2] << 16;
		k |= bdata[3] << 24;
#else
		uint k = *(uint*)bdata;
#endif

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		bdata += 4;
		len -= 4;
	}

	switch(len) {
		case 3: h ^= bdata[2] << 16;
		case 2: h ^= bdata[1] << 6;
		case 1: h ^= bdata[0];
				h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

