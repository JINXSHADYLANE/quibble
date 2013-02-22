#include "bind.h"

#include "lighting.h"

#include <darray.h>

static DArray lights_buffer;

#define checkargs(c, name) \
	int n = lua_gettop(l); \
	if(n != c) \
		return luaL_error(l, "wrong number of arguments provided to " name \
			"; got %d, expected " #c, n)

#define checktmaphandle(l, i) \
	(Tilemap**)luaL_checkudata(l, i, "_TilemapHandle.mt")

extern bool _check_vec2(lua_State* l, int i, Vector2* v);
extern bool _check_rect(lua_State* l, int i, RectF* r);
extern void _new_rect(lua_State* l, double _l, double t,
	double r, double b);
extern void _new_vec2(lua_State* l, double y, double x);

static int clighting_init(lua_State* l) {
	checkargs(1, "clighting.init");
	RectF screen;
	if(!_check_rect(l, 1, &screen))
		return luaL_error(l, "bad rect provided to clighting.init");
	lighting_init(screen);
	return 0;
}

static int clighting_close(lua_State* l) {
	checkargs(0, "clighting.close");
	lighting_close();
	return 0;
}

static int clighting_render(lua_State* l) {
	checkargs(2, "clighting.render");
	uint layer = luaL_checkinteger(l, 1);
	uint len = lua_objlen(l, 2);
	lights_buffer.size = 0;
	for(uint i = 0; i < len; ++i) {
		lua_rawgeti(l, 2, i+1);
		lua_getfield(l, 3, "pos");
		Vector2 pos;
		if(!_check_vec2(l, 4, &pos))
			return luaL_error(l, "unable to get light pos");
		lua_getfield(l, 3, "radius");
		float radius = luaL_checknumber(l, 5);
		lua_getfield(l, 3, "alpha");
		float alpha = luaL_checknumber(l, 6);
		lua_pop(l, 4);
		Light new = {pos, radius, alpha};
		darray_append(&lights_buffer, (void*)&new);
	}
	lighting_render(layer, &lights_buffer);
	return 0;
}

static const luaL_Reg clighting_fun[] = {
	{"init", clighting_init},
	{"close", clighting_close},
	{"render", clighting_render},
	{NULL, NULL}
};	

int bind_open_kovas(lua_State* l) {
	lights_buffer = darray_create(sizeof(Light), 0);

	luaL_register(l, "clighting", clighting_fun);

	return 1;
}

void bind_close_kovas(void) {
	darray_free(&lights_buffer);
}

