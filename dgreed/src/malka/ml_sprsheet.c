#include "ml_sprsheet.h"
#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <sprsheet.h>

#define checksprhandle(l, i) \
	(SprHandle*)luaL_checkudata(l, i, "_SprHandle.mt");

extern void _new_texhandle(lua_State* l, TexHandle h);
extern void _new_rect(lua_State* l, double _l, double t, 
		double r, double b);
extern bool _check_vec2(lua_State* l, int i, Vector2* v);
extern bool _check_rect(lua_State* l, int i, RectF* r);
extern bool _check_color(lua_State* l, int i, Color* c);

static void _new_sprhandle(lua_State* l, SprHandle h) {
	SprHandle* s = (SprHandle*)lua_newuserdata(l, sizeof(SprHandle));
	*s = h;
	luaL_getmetatable(l, "_SprHandle.mt");
	lua_setmetatable(l, -2);
}

static int ml_sprsheet_init(lua_State* l) {
	checkargs(1, "sprsheet.init");
	const char* filename = luaL_checkstring(l, 1);	

	sprsheet_init(filename);
	return 0;
}

static int ml_sprsheet_close(lua_State* l) {
	checkargs(0, "sprsheet.close");

	sprsheet_close();
	return 0;
}

static int ml_sprsheet_get_handle(lua_State* l) {
	checkargs(1, "sprsheet.get_handle");

	const char* name = luaL_checkstring(l, 1);
	SprHandle h = sprsheet_get_handle(name);
	_new_sprhandle(l, h);
	return 1;
}

#define resolvespr(l, i, name, handle) {\
	if(lua_isstring(l, i)) {name = lua_tostring(l, i); handle = NULL;} \
	else {name = NULL; handle = checksprhandle(l, i);}}

static int ml_sprsheet_get(lua_State* l) {
	checkargs(1, "sprsheet.get");

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	TexHandle tex;
	RectF r;
	if(name)
		sprsheet_get(name, &tex, &r);
	else
		sprsheet_get_h(*h, &tex, &r);

	_new_texhandle(l, tex);
	_new_rect(l, r.left, r.top, r.right, r.bottom);

	return 2;
}

static int ml_sprsheet_get_anim(lua_State* l) {
	checkargs(2, "sprsheet.get_anim");

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	uint frame = luaL_checkinteger(l, 2);

	TexHandle tex;
	RectF r;
	if(name)
		sprsheet_get_anim(name, frame, &tex, &r);
	else
		sprsheet_get_anim_h(*h, frame, &tex, &r);

	_new_texhandle(l, tex);
	_new_rect(l, r.left, r.top, r.right, r.bottom);

	return 2;
}

static int ml_sprsheet_anim_frames(lua_State* l) {
	checkargs(1, "sprsheet.anim_frames");

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	uint f;
	if(name)
		f = sprsheet_get_anim_frames(name);
	else
		f = sprsheet_get_anim_frames_h(*h);

	lua_pushinteger(l, f);

	return 1;
}

static int ml_sprsheet_draw(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 3 || n > 4)
		goto error;

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	uint layer = luaL_checkinteger(l, 2);

	Vector2 vdest;
	RectF dest = {0.0f, 0.0f, 0.0f, 0.0f};
	if(!_check_vec2(l, 3, &vdest)) {
		if(!_check_rect(l, 3, &dest))
			goto error;
	}
	else {
		dest.left = vdest.x;
		dest.top = vdest.y;
	}

	Color tint = COLOR_WHITE;
	if(n == 4)
		if(!_check_color(l, 4, &tint))
			goto error;

	if(name)
		spr_draw(name, layer, dest, tint);
	else
		spr_draw_h(*h, layer, dest, tint);

	return 0;
error:
	return luaL_error(l, "bad arguments to sprsheet.draw");
}

static int ml_sprsheet_draw_anim(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 4 || n > 5)
		goto error;

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	uint frame = luaL_checkinteger(l, 2);
	uint layer = luaL_checkinteger(l, 3);

	Vector2 vdest;
	RectF dest = {0.0f, 0.0f, 0.0f, 0.0f};
	if(!_check_vec2(l, 4, &vdest)) {
		if(!_check_rect(l, 4, &dest))
			goto error;
	}
	else {
		dest.left = vdest.x;
		dest.top = vdest.y;
	}

	Color tint = COLOR_WHITE;
	if(n == 5)
		if(!_check_color(l, 5, &tint))
			goto error;

	if(name)
		spr_draw_anim(name, frame, layer, dest, tint);
	else
		spr_draw_anim_h(*h, frame, layer, dest, tint);

	return 0;
error:
	return luaL_error(l, "bad arguments to sprsheet.draw_anim");
}

static int ml_sprsheet_draw_centered(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 3 || n > 6)
		goto error;

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	uint layer = luaL_checkinteger(l, 2);

	Vector2 dest;
	if(!_check_vec2(l, 3, &dest))
		goto error;

	float rot = 0.0f;
	float scale = 1.0f;
	Color tint = COLOR_WHITE;

	if(n == 4) {
		if(!lua_isnumber(l, 4)) {
			if(!_check_color(l, 4, &tint))
				goto error;
		}
		else {
			rot = lua_tonumber(l, 4);
		}
	}
	if(n == 5) {
		rot = lua_tonumber(l, 4);
		if(!lua_isnumber(l, 5)) {
			if(!_check_color(l, 5, &tint))
				goto error;
		}
		else {
			scale = lua_tonumber(l, 5);
		}
	}
	if(n == 6) {
		rot = lua_tonumber(l, 4);
		scale = lua_tonumber(l, 5);
		if(!_check_color(l, 6, &tint))
			goto error;
	}

	if(name)
		spr_draw_cntr(name, layer, dest, rot, scale, tint);
	else
		spr_draw_cntr_h(*h, layer, dest, rot, scale, tint);

	return 0;
error:
	return luaL_error(l, "bad arguments to sprsheet.draw_centered");
}

static int ml_sprsheet_draw_anim_centered(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 4 || n > 7)
		goto error;

	const char* name;
	SprHandle* h;
	resolvespr(l, 1, name, h);

	uint frame = luaL_checkinteger(l, 2);
	uint layer = luaL_checkinteger(l, 3);

	Vector2 dest;
	if(!_check_vec2(l, 4, &dest))
		goto error;

	float rot = 0.0f;
	float scale = 1.0f;
	Color tint = COLOR_WHITE;

	if(n == 5) {
		if(!lua_isnumber(l, 5)) {
			if(!_check_color(l, 5, &tint))
				goto error;
		}
		else {
			rot = lua_tonumber(l, 5);
		}
	}
	if(n == 6) {
		rot = lua_tonumber(l, 5);
		if(!lua_isnumber(l, 6)) {
			if(!_check_color(l, 6, &tint))
				goto error;
		}
		else {
			scale = lua_tonumber(l, 6);
		}
	}
	if(n == 7) {
		rot = lua_tonumber(l, 5);
		scale = lua_tonumber(l, 6);
		if(!_check_color(l, 7, &tint))
			goto error;
	}

	if(name)
		spr_draw_anim_cntr(name, frame, layer, dest, rot, scale, tint);
	else
		spr_draw_anim_cntr_h(*h, frame, layer, dest, rot, scale, tint);

	return 0;
error:
	return luaL_error(l, "bad arguments to sprsheet.draw_anim_centered");
}

static const luaL_Reg sprsheet_fun[] = {
	{"init", ml_sprsheet_init},
	{"close", ml_sprsheet_close},
	{"get_handle", ml_sprsheet_get_handle},
	{"get", ml_sprsheet_get},
	{"get_anim", ml_sprsheet_get_anim},
	{"anim_frames", ml_sprsheet_anim_frames},
	{"draw", ml_sprsheet_draw},
	{"draw_anim", ml_sprsheet_draw_anim},
	{"draw_centered", ml_sprsheet_draw_centered},
	{"draw_anim_centered", ml_sprsheet_draw_anim_centered},
	{NULL, NULL}
};

int malka_open_sprsheet(lua_State* l) {
	luaL_newmetatable(l, "_SprHandle.mt");
	luaL_register(l, "sprsheet", sprsheet_fun);

	lua_pop(l, 2);

	return 1;
}

