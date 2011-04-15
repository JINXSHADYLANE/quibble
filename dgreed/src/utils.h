#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

/*
--------------------
--- Common types ---
--------------------
*/

typedef long long int64;
typedef unsigned long long uint64;
typedef int int32;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef short int16;
typedef unsigned short uint16;
typedef char int8;  
typedef unsigned char uint8;
typedef unsigned char byte;

#define MIN_INT8 ((int8) 0x80)
#define MAX_INT8 ((int8) 0x7f)
#define MAX_UINT8 ((uint8) 0xff)

#define MIN_INT16 ((int16) 0x8000)
#define MAX_INT16 ((int16) 0x7fff)
#define MAX_UINT16 ((uint16) 0xffff)

#define MIN_INT32 ((int32) 0x80000000)
#define MAX_INT32 ((int32) 0x7fffffff)
#define MAX_UINT32 ((uint32) 0xffffffff)

#define MIN_INT64 ((int64) 0x8000000000000000LL)
#define MAX_INT64 ((int64) 0x7fffffffffffffffLL)
#define MAX_UINT64 ((uint64) 0xffffffffffffffffULL)

#define PI 3.141592653f
#define RAD_TO_DEG (180.0f / PI)
#define DEG_TO_RAD (1.0f / RAD_TO_DEG)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

/*
-------------------
--- Vector math ---
-------------------
*/

typedef struct {
	float x, y;
} Vector2;	

// Vector2 constructor
Vector2 vec2(float x, float y);

// Simple arithmetic
Vector2 vec2_add(Vector2 a, Vector2 b);
Vector2 vec2_sub(Vector2 a, Vector2 b);
Vector2 vec2_scale(Vector2 a, float b);
Vector2 vec2_normalize(Vector2 a);
Vector2 vec2_rotate(Vector2 a, float angle);
float vec2_dot(Vector2 a, Vector2 b);
float vec2_length(Vector2 a);
float vec2_length_sq(Vector2 b);
float vec2_dir(Vector2 a);
float vec2_angle(Vector2 a, Vector2 b);

/*
-----------------
--- Rectangle ---
-----------------
*/

typedef struct {
	float left, top, right, bottom;
} RectF;	

// Returns {0, 0, 0, 0} rect
RectF rectf_null(void);
// RectF constructor
RectF rectf(float left, float top, float right, float bottom);
// Returns true if point is in rectangle
bool rectf_contains_point(const RectF* r, const Vector2* p);
// Returns true if rectangle and circle collide
bool rectf_circle_collision(const RectF* rect, const Vector2* p, float r);
// Returns true if two rectangles intersect
bool rectf_rectf_collision(const RectF* rect1, const RectF* rect2);
// Returns signed width/height
float rectf_width(const RectF* r);
float rectf_height(const RectF* r);
// Returns center
Vector2 rectf_center(const RectF* r);
// Returns first intersection point or end
Vector2 rectf_raycast(const RectF* r, const Vector2* start, const Vector2* end);

/*
----------------
--- Triangle ---
----------------
*/

typedef struct {
	Vector2 p1, p2, p3;
} Triangle;	

// Returns true if point is inside triangle
bool tri_contains_point(const Triangle* tri, const Vector2* p);
// Returns true if rectangle and triangle intersect
bool tri_rectf_collision(const Triangle* tri, const RectF* r); 

/*
--------------------
--- Line segment ---
--------------------
*/

typedef struct {
	Vector2 p1, p2;
} Segment;	

// Constructor
Segment segment(Vector2 p1, Vector2 p2);

// Returns length of segment
float segment_length(Segment s);

// Returns signed distance from line segment to a point.
// Distance is negative if point is to the right of segment 
// (assuming segment goes up from p1 to p2)
float segment_point_dist(Segment s, Vector2 p); 

// Returns true if segments intersect,
// optionally sets intersection point if p is not null.
bool segment_intersect(Segment s1, Segment s2, Vector2* p);

/*
--------------
--- Colors ---
--------------
*/

#define COLOR_RGBA(r, g, b, a) \
	(((r)&0xFF)|(((g)&0xFF)<<8)|(((b)&0xFF)<<16)|(((a)&0xFF)<<24))
#define COLOR_DECONSTRUCT(color, r, g, b, a) \
	(r) = color & 0xFF; \
	(g) = (color & 0xFF00) >> 8; \
	(b) = (color & 0xFF0000) >> 16; \
	(a) = (color & 0xFF000000) >> 24
#define COLOR_WHITE COLOR_RGBA(255, 255, 255, 255)
#define COLOR_BLACK COLOR_RGBA(0, 0, 0, 255)
#define COLOR_TRANSPARENT COLOR_RGBA(255, 255, 255, 0)

typedef uint Color;

typedef struct {
	float h, s, v, a;
} ColorHSV;

Color hsv_to_rgb(ColorHSV hsv);
ColorHSV rgb_to_hsv(Color rgb);
Color color_lerp(Color c1, Color c2, float t);

/*
----------------------
--- Random numbers ---
----------------------
*/

// Initializes randomizer with seed
void rand_init(uint seed);

// Returns random uint in range [0, max_uint]
uint rand_uint(void);

// Returns random int in specified range
int rand_int(int min, int max);

// Returns random float in range [0.0f, 1.0f]
float rand_float(void);

// Returns random float in specified range
float rand_float_range(float min, float max);

/*
---------------
--- Logging ---
---------------
*/

#define LOG_MSG_BUFFER_SIZE 512

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2

// Logging functions
void LOG_ERROR(const char* format, ...);
void LOG_WARNING(const char* format, ...);
void LOG_INFO(const char* format, ...);
 
// Initializes logging to specified file
// Use log_level parameter to modify verbosity of output
bool log_init(const char* log_path, uint log_level);

// Closes log file
void log_close(void);

// Internal logging function, don't use this
void log_send(uint log_level, const char* format, va_list args); 

/*
---------------------------
--- Parameters handling ---
---------------------------
*/

// Initializes parameter handling system
void params_init(uint argc, const char** argv);

// Returns number of parameters (argc)
uint params_count(void);

// Returns n-th parameter
const char* params_get(uint n);

// Searches for specified parameter, returns its position, 
// or 0 if there is no such parameter 
uint params_find(const char* param);

/*
---------------
--- File IO ---
---------------
*/

typedef size_t FileHandle;

// Returns true if file exists
bool file_exists(const char* name);

// Open file for reading
FileHandle file_open(const char* name);
// Close opened file
void file_close(FileHandle f);

// Get file size in bytes
uint file_size(FileHandle f);
// Move read pointer to specified position, relative to beginning of file
void file_seek(FileHandle f, uint pos);

// Reading functions
byte file_read_byte(FileHandle f);
uint16 file_read_uint16(FileHandle f);
uint32 file_read_uint32(FileHandle f);
float file_read_float(FileHandle f);
void file_read(FileHandle f, void* dest, uint size);

// Opens/creates new file for writing
FileHandle file_create(const char* name);

// Writing functions
void file_write_byte(FileHandle f, byte data);
void file_write_uint16(FileHandle f, uint16 data);
void file_write_uint32(FileHandle f, uint32 data);
void file_write_float(FileHandle f, float data);
void file_write(FileHandle f, void* data, uint size);

// Text file helpers

// Saves text string to a file
void txtfile_write(const char* name, const char* text);
// Loads file to a text string. You must free allocated buffer yourself!
char* txtfile_read(const char* name);

// File path manipulation

// Returns real path to resource, use it when operating on files with 
// classic IO procedures.
char* path_to_resource(const char* file);

// Returns new string with removed file name from path. Result is NULL
// if path is invalid (eg. with already removed filename).
char* path_get_folder(const char* path);

// Returns same path, with changed file extension. If path does
// not end with file or extension is invalid returns NULL. 
char* path_change_ext(const char* path, const char* ext);

/*
------------
--- Misc ---
------------
*/

// Constructs FourCC code
#define FOURCC(a, b, c, d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))

// Returns amount of items in static array
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// Converts 2d array index to 1d index
#define IDX_2D(x, y, width) ((y)*(width) + (x))

// Reorders bytes to little-endian format
uint16 endian_swap2(uint16 in);
uint32 endian_swap4(uint32 in);

// Copies string to some new place in memory and returns pointer to it.
// You must free this pointer!
#ifdef TRACK_MEMORY
char* strclone_tracked(const char* str, const char* file, int line);
#define strclone(str) strclone_tracked(str, __FILE__, __LINE__)
#else
char* strclone_untracked(const char* str);
#define strclone(str) strclone_untracked(str)
#endif

// lerp
float lerp(float a, float b, float t);

// Smoothstep
float smoothstep(float a, float b, float t);

// Normalizes value to 0 - 1 range
float normalize(float val, float min, float max);

// Clamp value in specified interval
float clamp(float min, float max, float val);

// Returns true if floats are within epsilon range
bool feql(float a, float b); 

// Returns true if integer is a power of two
bool is_pow2(uint n);

/*
-------------------
--- Compression ---
-------------------
*/

// Compresses input buffer with lzss algorithm 
// You have to free memory allocated for output buffer yourself!
void* lz_compress(void* input, uint input_size, uint* output_size);

// Decompresses buffer which was compressed with lz_compress 
// You have to free memory allocated for output buffer yourself! 
void* lz_decompress(void* input, uint input_size, uint* output_size);

/*
--------------
--- Base64 ---
--------------
*/

// Encodes binary data with base64; you must free returned buffer
char* base64_encode(const void* input, uint input_size, uint* output_size);

// Decodes base64-encoded data; you must free returned buffer
void* base64_decode(const char* input, uint input_isze, uint* output_size);

/*
---------------
--- Hashing ---
---------------
*/

// Fast and simple hashing algorithm with funny name
// http://en.wikipedia.org/wiki/MurmurHash
uint hash_murmur(const void* data, uint len, uint seed);

#endif

