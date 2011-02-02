#include "bind.h"

#include "objects.h"
#include "lighting.h"

#include <darray.h>

static DArray get_buffer;
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

static int cobjects_reset(lua_State* l) {
	checkargs(1, "cobjects.reset");
	Tilemap** t = checktmaphandle(l, 1);

	objects_reset(*t);
	return 0;
}

static int cobjects_close(lua_State* l) {
	checkargs(0, "cobjects.close");
	objects_close();
	return 0;
}

static int cobjects_add(lua_State* l) {
	checkargs(2, "cobjects.add");
	uint type = luaL_checkinteger(l, 1);
	Vector2 pos;
	if(!_check_vec2(l, 2, &pos))
		return luaL_error(l, "bad second argument to cobjects.add");
	
	assert(type < obj_count);
	objects_add(type, pos);
	return 0;
}

static int cobjects_seal(lua_State* l) {
	checkargs(1, "cobjects.seal");
	RectF player_rect;
	if(!_check_rect(l, 1, &player_rect))
		return luaL_error(l, "bad rect provided to cobjects.seal");

	objects_seal(player_rect);
	return 0;
}

static int cobjects_move_player(lua_State* l) {
	checkargs(1, "cobjects.move_player");
	Vector2 offset;
	if(!_check_vec2(l, 1, &offset))
		return luaL_error(l, "bad first argument to cobjects.add");
	bool battery;
	RectF bbox = objects_move_player(offset, &battery);
	_new_rect(l, (double)bbox.left, (double)bbox.top,
		(double)bbox.right, (double)bbox.bottom);
	lua_pushboolean(l, battery);
	return 2;
}

static int cobjects_get_crates(lua_State* l) {
	checkargs(1, "cobjects.get_crates");
	RectF screen;
	if(!_check_rect(l, 1, &screen))
		return luaL_error(l, "bad rect provided to cobjects.get_crates");

	objects_get(obj_crate, screen, &get_buffer);
	Object* objs = DARRAY_DATA_PTR(get_buffer, Object);
	lua_createtable(l, get_buffer.size, 0);
	for(int i = 0; i < get_buffer.size; ++i) {
		assert(objs[i].type == obj_crate);
		Vector2 p = vec2(objs[i].rect.left, objs[i].rect.top); 
		_new_vec2(l, (double)p.x, (double)p.y); 
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static int cobjects_get_beacons(lua_State* l) {
	checkargs(1, "cobjects.get_beacons");
	RectF screen;
	if(!_check_rect(l, 1, &screen))
		return luaL_error(l, "bad rect provided to cobjects.get_beacons");

	objects_get(obj_beacon, screen, &get_buffer);
	Object* objs = DARRAY_DATA_PTR(get_buffer, Object);
	lua_createtable(l, get_buffer.size, 0);
	for(int i = 0; i < get_buffer.size; ++i) {
		lua_createtable(l, 0, 2);
		assert(objs[i].type == obj_beacon);
		Vector2 p = vec2(objs[i].rect.left, objs[i].rect.top); 
		_new_vec2(l, (double)p.x, (double)p.y); 
		lua_setfield(l, -2, "pos");
		lua_pushnumber(l, objs[i].data.beacon_intensity);
		lua_setfield(l, -2, "intensity");
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static int cobjects_get_batteries(lua_State* l) {
	checkargs(1, "cobjects.get_batteries");
	RectF screen;
	if(!_check_rect(l, 1, &screen))
		return luaL_error(l, "bad rect provided to cobjects.get_batteries");

	objects_get(obj_battery, screen, &get_buffer);
	Object* objs = DARRAY_DATA_PTR(get_buffer, Object);
	lua_createtable(l, get_buffer.size, 0);
	for(int i = 0; i < get_buffer.size; ++i) {
		assert(objs[i].type == obj_battery);
		Vector2 p = vec2(objs[i].rect.left, objs[i].rect.top); 
		_new_vec2(l, (double)p.x, (double)p.y); 
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static int cobjects_get_buttons(lua_State* l) {
	checkargs(1, "cobjects.get_buttons");
	RectF screen;
	if(!_check_rect(l, 1, &screen))
		return luaL_error(l, "bad rect provided to cobjects.get_buttons");

	objects_get(obj_button, screen, &get_buffer);
	Object* objs = DARRAY_DATA_PTR(get_buffer, Object);
	lua_createtable(l, get_buffer.size, 0);
	for(int i = 0; i < get_buffer.size; ++i) {
		lua_createtable(l, 0, 2);
		assert(objs[i].type == obj_button);
		Vector2 p = vec2(objs[i].rect.left, objs[i].rect.top); 
		_new_vec2(l, (double)p.x, (double)p.y); 
		lua_setfield(l, -2, "pos");
		lua_pushboolean(l, objs[i].data.button_state);
		lua_setfield(l, -2, "state");
		lua_rawseti(l, -2, i+1);
	}
	return 1;
}

static const luaL_Reg cobjects_fun[] = {
	{"reset", cobjects_reset},
	{"close", cobjects_close},
	{"add", cobjects_add},
	{"seal", cobjects_seal},
	{"move_player", cobjects_move_player},
	{"get_crates", cobjects_get_crates},
	{"get_beacons", cobjects_get_beacons},
	{"get_batteries", cobjects_get_batteries},
	{"get_buttons", cobjects_get_buttons},
	{NULL, NULL}
};	

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

int bind_open_bulb(lua_State* l) {
	get_buffer = darray_create(sizeof(Object), 0);
	lights_buffer = darray_create(sizeof(Light), 0);

	luaL_register(l, "cobjects", cobjects_fun);
	luaL_register(l, "clighting", clighting_fun);

	lua_getglobal(l, "cobjects");
	int tbl = lua_gettop(l);
	lua_pushinteger(l, 0);
	lua_setfield(l, tbl, "obj_crate");
	lua_pushinteger(l, 1);
	lua_setfield(l, tbl, "obj_beacon");
	lua_pushinteger(l, 2);
	lua_setfield(l, tbl, "obj_button");
	lua_pushinteger(l, 3);
	lua_setfield(l, tbl, "obj_battery");

	return 1;
}

void bind_close_bulb(void) {
	darray_free(&get_buffer);
	darray_free(&lights_buffer);
}

