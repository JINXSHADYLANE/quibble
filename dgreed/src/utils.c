#include "utils.h"
#include "memory.h"

#include <ctype.h>

#ifdef MACOSX_BUNDLE
#include <CoreFoundation/CFBundle.h>

CFBundleRef main_bundle;
extern const char* g_home_dir;

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

	CFURLRef resource_url = CFBundleCopyResourceURL(main_bundle,
		CFStringCreateWithCString(NULL, filename, kCFStringEncodingASCII),
		CFStringCreateWithCString(NULL, extension, kCFStringEncodingASCII),
		CFStringCreateWithCString(NULL, file, kCFStringEncodingASCII));

	if(!resource_url) {
		LOG_WARNING("Unable to get url to file %s in a bundle", file);
		MEM_FREE(file);
		return NULL;
	}	

	char* path = MEM_ALLOC(sizeof(char) * 512);
	if(!CFURLGetFileSystemRepresentation(resource_url, true, (UInt8*)path, 512))
		LOG_ERROR("Unable to turn url into path");

	MEM_FREE(file);
	return path;	
}
#else
char* path_to_resource(const char* _file) {
	return strclone(_file);
}
#endif

/*
-------------------
--- Vector math ---
-------------------
*/

Vector2 vec2(float x, float y) {
	Vector2 result = {x, y};
	return result;
}

Vector2 vec2_add(Vector2 a, Vector2 b) {
	return vec2(a.x + b.x, a.y + b.y);
}

Vector2 vec2_sub(Vector2 a, Vector2 b) {
	return vec2(a.x - b.x, a.y - b.y);
}

Vector2 vec2_scale(Vector2 a, float b) {
	return vec2(a.x * b, a.y * b);
}

Vector2 vec2_normalize(Vector2 a) {
	float inv_len = 1.0f / vec2_length(a);
	return vec2_scale(a, inv_len);
}	

Vector2 vec2_rotate(Vector2 a, float angle) {
	Vector2 result;
	float s = sin(angle * DEG_TO_RAD);
	float c = cos(angle * DEG_TO_RAD);
	result.x = c * a.x - s * a.y;
	result.y = s * a.x + c * a.y;
	return result;
}	

float vec2_dot(Vector2 a, Vector2 b) {
	return a.x * b.x + a.y * b.y;
}

float vec2_length(Vector2 a) {
	return sqrt(a.x*a.x + a.y*a.y);
}	

float vec2_length_sq(Vector2 a) {
	return a.x*a.x + a.y*a.y;
}	

float vec2_dir(Vector2 a) {
	Vector2 norm_a = vec2_normalize(a);
	return acos(norm_a.x) * RAD_TO_DEG;
}	

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

bool rectf_circle_collision(const RectF* rect, const Vector2* p, float r) {
	assert(r >= 0.0f && r < 1000000.0f);
	float d1, d2, d3, d4;
	d1 = d2 = d3 = d4 = 1000000.0f;
	if(p->x >= rect->left && p->x <= rect->right) {
		d1 = abs(rect->top - p->y);
		d2 = abs(p->y - rect->bottom);
	}
	if(p->y <= rect->bottom && p->y >= rect->top) {
		d3 = abs(rect->left - p->x);
		d4 = abs(p->x - rect->right);
	}
	if(MIN(MIN(d1, d2), MIN(d3, d4)) <= r)
		return true;

	d1 = vec2_length_sq(vec2_sub(*p, vec2(rect->left, rect->top)));	
	d2 = vec2_length_sq(vec2_sub(*p, vec2(rect->left, rect->bottom)));	
	d3 = vec2_length_sq(vec2_sub(*p, vec2(rect->right, rect->top)));	
	d4 = vec2_length_sq(vec2_sub(*p, vec2(rect->right, rect->bottom)));	

	if(MIN(MIN(d1, d2), MIN(d3, d4)) <= r*r)
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
	if(abs(det) < epsilon)
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

void rand_init(uint seed) {
	srand(seed);
}

uint rand_uint(void) {
	uint16 r1 = rand();
	uint16 r2 = rand();
	uint16 r3 = rand();
	return (r1 & 0xFFF) | ((r2 & 0xFFF) << 12) | ((r3 & 0xFF) << 24); 
}

int rand_int(int min, int max) {
	int range;

	assert(min < max);

	range = max - min;
	return (rand_uint()%range) + min;
}

float rand_float(void) {
	return (float)rand_uint() / (float)MAX_UINT32;
}

float rand_float_range(float min, float max) {
	return min + rand_float() * (max - min);
}

/*
---------------
--- Logging ---
---------------
*/

FILE* log_file = NULL;
uint log_level = LOG_LEVEL_INFO;

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

bool log_init(const char* log_path, uint log_level) {
#ifdef MACOSX_BUNDLE
	char bundle_log_path[512];
	sprintf(bundle_log_path, "%s/Library/Logs/%s", g_home_dir, log_path);
	log_file = fopen(bundle_log_path, "w");
#else
	log_file = fopen(log_path, "w");
#endif	
	if(log_file == NULL)
			return false;

	log_level = log_level;	

	LOG_INFO("Log initialized");
	return true;
}

void log_close(void) {
	assert(log_file);

	LOG_INFO("Log closed");
	fclose(log_file);
}

/* TODO: Display time */
void log_send(uint log_level, const char* format, va_list args) {
	static char msg_buffer[LOG_MSG_BUFFER_SIZE];

	assert(log_file);

	if(log_level < log_level)
			return;

	vsnprintf(msg_buffer, LOG_MSG_BUFFER_SIZE, format, args);
	fprintf(log_file, "%s: %s\n", log_level_to_cstr(log_level), msg_buffer);
}

/*
---------------------------
--- Parameters handling ---
---------------------------
*/

uint largc = 0;
const char** largv = NULL;

void params_init(uint argc, const char** argv) {
	assert(argv);

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

	assert(largv);

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

bool file_exists(const char* name) {
	assert(name);

#ifdef MACOSX_BUNDLE
	char* path = get_resource_path(name);
	if(path == NULL)
		return false;
	FILE* file = fopen(path, "rb");
	MEM_FREE(path);
#else	
	FILE* file = fopen(name, "rb");
#endif

	if(file != NULL) {
		fclose(file);
		return true;
	}
	return false;
}

FileHandle file_open(const char* _name) {
	assert(_name);

	char* name = strclone(_name);

	// Filter out newlines
	uint i = 0;
	while(name[i]) {
		if(name[i] == '\n')
			name[i] = '\0';
		++i;
	}

#ifdef MACOSX_BUNDLE
	char* path = get_resource_path(name);
	if(path == NULL) {
		LOG_ERROR("Unable to get path to file %s", name);
		return 0;
	}	
	FILE* f = fopen(path, "rb");
	MEM_FREE(path);
#else
	FILE* f = fopen(name, "rb");
#endif

	MEM_FREE(name);

	if(f == NULL) {
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

	FILE* file = fopen(name, "wb");
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

void file_write(FileHandle f, void* data, uint size) {
	FILE* file = (FILE*)f;

	assert(file);

	if(fwrite(data, 1, size, file) != size)
		LOG_ERROR("File writing error");
}

void txtfile_write(const char* name, const char* text) {
	assert(name);
	assert(text);

	FILE* file = fopen(name, "w");

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

int endianess_helper = 0xdeadcafe;

uint16 endian_swap2(uint16 in) {
	uint16 helper = 0x0011;
	byte first_byte = *(byte*)&helper;
	if(first_byte == 0x00) // Big endian
		return ((in << 8) & 0xFF00) | ((in >> 8) & 0xFF);
	else	
		return in;
}

uint32 endian_swap4(uint32 in) {
	uint32 helper = 0x00112233;
	byte first_byte = *(byte*)&helper;
	if(first_byte == 0x00) // Big endian
		return ((in << 24) & 0xFF000000) | ((in << 8) & 0x00FF0000) | \
			((in >> 8) & 0x0000FF00) | ((in >> 24) & 0x000000FF);
	if(first_byte == 0x33) // Little endian
		return in;
	assert(0 && "Unable to determine cpu endianess");	
	return 0;	
}

char* strclone(const char* str) {
	assert(str);

	uint len = strlen(str);
	char* clone = MEM_ALLOC(len+1);
	strcpy(clone, str);
	return clone;
}	

float lerp(float a, float b, float t) {
	 return a + (b - a) * t;
}	 

float smoothstep(float a, float b, float t) {
	return lerp(a, b, t * t * (3.0f - 2.0f * t));
}	

float clamp(float min, float max, float val) {
	assert(min < max);
	return MAX(min, MIN(max, val));
}	

bool feql(float a, float b) {
	return abs(a - b) < 0.00001f;
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
---------------
--- Hashing ---
---------------
*/

uint hash_murmur(void* data, uint len, uint seed) {
	const uint m = 0x5bd1e995;
	const int r = 24;
	uint h = seed ^ len;
	const byte* bdata = (const byte*)data;

	while(len >= 4) {
		uint k = bdata[0];
		k |= bdata[1] << 8;
		k |= bdata[2] << 16;
		k |= bdata[3] << 24;

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

