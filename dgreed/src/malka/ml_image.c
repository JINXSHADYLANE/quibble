#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <image.h>
#include <memory.h>

typedef struct {
	void* pixels;
	uint width, height;
	PixelFormat format;
} Image;

static int ml_img_load(lua_State* l) {
	checkargs(1, "img.load");
	const char* filename = luaL_checkstring(l, 1);

	uint w, h;
	PixelFormat fmt;

	void* data = image_load(filename, &w, &h, &fmt);
	if(data) {
		Image* new = MEM_ALLOC(sizeof(Image));
		new->pixels = data;
		new->width = w;
		new->height = h;
		new->format = fmt;
		lua_pushlightuserdata(l, new);
	}
	else {
		lua_pushnil(l);
	}

	return 1;
}

extern void _new_vec2(lua_State* l, double x, double y);

static int ml_img_size(lua_State* l) {
	checkargs(1, "img.size");
	assert(lua_islightuserdata(l, 1));

	Image* img = lua_touserdata(l, 1);
	_new_vec2(l,
		(double)img->width,
		(double)img->height
	);

	return 1;
}

static int ml_img_free(lua_State* l) {
	checkargs(1, "img.free");
	assert(lua_islightuserdata(l, 1));

	Image* img = lua_touserdata(l, 1);

	free(img->pixels);
	MEM_FREE(img);

	return 0;
}

extern void _new_rgba(lua_State* l, double r, double g,
	double b, double a);

static int ml_img_pixel(lua_State* l) {
	checkargs(3, "img.free");
	assert(lua_islightuserdata(l, 1));

	Image* img = lua_touserdata(l, 1);
	int x = luaL_checkinteger(l, 2);
	int y = luaL_checkinteger(l, 2);

	if(x < 0 || x >= img->width || y < 0 || y >= img->height)
		return luaL_error(l, "pixel coordinates out of image bounds");
	if(img->format != PF_RGBA8888)
		return luaL_error(l, "unsupported image pixel format");

	Color* img_color = (Color*)img;
	Color c = img_color[y * img->width + x];

	byte r, g, b, a;
	COLOR_DECONSTRUCT(c, r, g, b, a);

	double inv_mul = 1.0 / 255.0;

	_new_rgba(l,
		(double)r * inv_mul,
		(double)g * inv_mul,
		(double)b * inv_mul,
		(double)a * inv_mul
	);

	return 1;
}

static const luaL_Reg img_fun[] = {
	{"load", ml_img_load},
	{"free", ml_img_free},
	{"size", ml_img_size},
	{"pixel", ml_img_pixel},
	{NULL, NULL}
};

int malka_open_img(lua_State* l) {
	malka_register(l, "img", img_fun);
	return 1;
}

