#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <coldet.h>

extern void _new_vec2(lua_State* l, double x, double y);
extern void _new_rect(lua_State* l, double _l, double t, 
		double r, double b);

extern bool _check_vec2(lua_State* l, int i, Vector2* v);
extern bool _check_rect(lua_State* l, int i, RectF* r);

#define checkcdworld(l, i) \
	(CDWorld*)luaL_checkudata(l, i, "_CDWorld.mt")

static void _new_cdworld(lua_State* l, float max_obj_size) {
	CDWorld* new = (CDWorld*)lua_newuserdata(l, sizeof(CDWorld));
	coldet_init(new, max_obj_size);
	luaL_getmetatable(l, "_CDWorld.mt");
	lua_setmetatable(l, -2);
}

static void _new_cdworld_ex(lua_State* l, float max_obj_size,
		float width, float height, bool horiz_wrap, bool vert_wrap) {
	CDWorld* new = (CDWorld*)lua_newuserdata(l, sizeof(CDWorld));
	coldet_init_ex(new, max_obj_size, width, height, horiz_wrap, vert_wrap);
	luaL_getmetatable(l, "_CDWorld.mt");
	lua_setmetatable(l, -2);
}

#define checkcdobj(l, i) \
	(CDObj**)luaL_checkudata(l, i, "_CDObj.mt")

static void _new_cdobj(lua_State* l, CDObj* obj) {
	CDObj** new = (CDObj**)lua_newuserdata(l, sizeof(CDObj*));
	*new = obj;
	luaL_getmetatable(l, "_CDObj.mt");
	lua_setmetatable(l, -2);
}

static int ml_coldet_init(lua_State* l) {
	int n = lua_gettop(l);

	float max_obj_size = luaL_checknumber(l, 1);
	if(n == 1) {
		_new_cdworld(l, max_obj_size);
		return 1;
	}
	if(n == 3) {
		float width = luaL_checknumber(l, 2);
		float height = luaL_checknumber(l, 3);
		bool horiz_wrap = width != 0.0f;
		bool vert_wrap = height != 0.0f;
		_new_cdworld_ex(l, max_obj_size, width, height, horiz_wrap, vert_wrap);
		return 1;
	}

	return luaL_error(l, "bad args to coldet.init");
}

static int ml_coldet_close(lua_State* l) {
	checkargs(1, "coldet.close");

	CDWorld* cd = checkcdworld(l, 1);
	coldet_close(cd);
	return 0;
}

static int ml_coldet_new_circle(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 3 || n > 5)
		goto error;
	CDWorld* cd = checkcdworld(l, 1);

	Vector2 center;
	if(!_check_vec2(l, 2, &center))
		goto error;

	float radius = luaL_checknumber(l, 3);

	uint mask = 0xFFFFFFFF;
	if(n == 4 || n == 5)
		mask = luaL_checkinteger(l, 4);

	void* userdata = NULL;
	if(n == 5) 
		userdata = (void*)(size_t)(lua_ref(l, 1));	

	CDObj* obj = coldet_new_circle(cd, center, radius, mask, userdata);
	_new_cdobj(l, obj);

	return 1;

error:
	return luaL_error(l, "bad args to coldet.new_circle");
}

static int ml_coldet_new_aabb(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 2 || n > 4)
		goto error;
	CDWorld* cd = checkcdworld(l, 1);

	RectF rect;
	if(!_check_rect(l, 2, &rect))
		goto error;

	uint mask = 0xFFFFFFFF;
	if(n == 3 || n == 4)
		mask = luaL_checkinteger(l, 4);

	void* userdata = NULL;
	if(n == 4) 
		userdata = (void*)(size_t)(lua_ref(l, 1));	

	CDObj* obj = coldet_new_aabb(cd, &rect, mask, userdata);
	_new_cdobj(l, obj);

	return 1;

error:
	return luaL_error(l, "bad args to coldet.new_aabb");
}

static int ml_coldet_remove(lua_State* l) {
	checkargs(2, "coldet.remove");

	CDWorld* cd = checkcdworld(l, 1);
	CDObj** obj = checkcdobj(l, 2);

	coldet_remove_obj(cd, *obj);

	return 0;
}

static lua_State* _ml_query_cb_l;
static void _ml_query_cb(CDObj* obj) {
	lua_pushvalue(_ml_query_cb_l, -1);
	_new_cdobj(_ml_query_cb_l, obj);
	lua_call(_ml_query_cb_l, 1, 0);
}

static int ml_coldet_query_circle(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 3 || n > 5)
		goto error;

	CDWorld* cd = checkcdworld(l, 1);

	Vector2 center;
	if(!_check_vec2(l, 2, &center))
		goto error;

	float radius = luaL_checknumber(l, 3);

	uint mask = 0xFFFFFFFF;
	if(n > 3) {
		if(lua_isnumber(l, 4)) {
			mask = luaL_checkinteger(l, 4);
		}
	}
	
	int res;
	if(n == 4 || n == 5) {
		_ml_query_cb_l = l;
		assert(lua_isfunction(l, -1));
		res = coldet_query_circle(cd, center, radius, mask, _ml_query_cb);
	}
	else {
		res = coldet_query_circle(cd, center, radius, mask, NULL);
	}

	lua_pushnumber(l, res);

	return 1;

error:
	return luaL_error(l, "bad args to coldet.query_circle");
}

static int ml_coldet_query_aabb(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 2 || n > 4)
		goto error;

	CDWorld* cd = checkcdworld(l, 1);

	RectF rect;
	if(!_check_rect(l, 2, &rect))
		goto error;

	uint mask = 0xFFFFFFFF;
	if(n > 2) {
		if(lua_isnumber(l, 3)) {
			mask = luaL_checkinteger(l, 3);
		}
	}
	
	int res;
	if(n == 3 || n == 4) {
		_ml_query_cb_l = l;
		assert(lua_isfunction(l, -1));
		res = coldet_query_aabb(cd, &rect, mask, _ml_query_cb);
	}
	else {
		res = coldet_query_aabb(cd, &rect, mask, NULL);
	}

	lua_pushnumber(l, res);

	return 1;

error:
	return luaL_error(l, "bad args to coldet.query_aabb");
}

static int ml_coldet_cast_segment(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 3 && n != 4)
		goto error;

	CDWorld* cd = checkcdworld(l, 1);

	Vector2 start, end;
	if(!_check_vec2(l, 2, &start) || !_check_vec2(l, 3, &end))
		goto error;

	uint mask = 0xFFFFFFFF;
	if(n == 4)
		mask = luaL_checkinteger(l, 4);

	Vector2 hitp;
	CDObj* obj = coldet_cast_segment(cd, start, end, mask, &hitp);

	_new_vec2(l, hitp.x, hitp.y);

	if(obj) {
		_new_cdobj(l, obj);
		return 2;
	}

	return 1;

error:
	return luaL_error(l, "bad args to coldet.cast_segment");
}

static void _ml_collide_cb(CDObj* a, CDObj* b) {
	lua_pushvalue(_ml_query_cb_l, -1);
	_new_cdobj(_ml_query_cb_l, a);
	_new_cdobj(_ml_query_cb_l, b);
	lua_call(_ml_query_cb_l, 2, 0);
}

static int ml_coldet_process(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1 && n != 2)
		goto error;

	CDWorld* cd = checkcdworld(l, 1);

	if(n == 2) {
		_ml_query_cb_l = l;
		assert(lua_isfunction(l, -1));
		coldet_process(cd, _ml_collide_cb);
	}
	else {
		coldet_process(cd, NULL);
	}

	return 0;

error:
	return luaL_error(l, "bad args to coldet.process");
}

static int ml_cdobj_is_circle(lua_State* l) {
	checkargs(1, "cdobj.is_circle");

	CDObj** pobj = checkcdobj(l, 1);
	lua_pushboolean(l, (*pobj)->type == CD_CIRCLE);
	return 1;
}

static int ml_cdobj_is_aabb(lua_State* l) {
	checkargs(1, "cdobj.is_aabb");

	CDObj** pobj = checkcdobj(l, 1);
	lua_pushboolean(l, (*pobj)->type == CD_AABB);
	return 1;
}

static int ml_cdobj_pos(lua_State* l) {
	checkargs(1, "cdobj.pos");

	CDObj* obj = *checkcdobj(l, 1);

	_new_vec2(l, obj->pos.x, obj->pos.y);
	return 1;
}

static int ml_cdobj_set_pos(lua_State* l) {
	checkargs(1, "cdobj.set_pos");

	CDObj* obj = *checkcdobj(l, 1);

	Vector2 pos;
	if(!_check_vec2(l, 2, &pos))
		return luaL_error(l, "bad second arg to cdobj.set_pos");

	obj->pos = pos;
	obj->dirty = true;

	return 0;
}

static int ml_cdobj_move(lua_State* l) {
	checkargs(2, "cdobj.move");

	CDObj* obj = *checkcdobj(l, 1);

	Vector2 off;
	if(!_check_vec2(l, 2, &off))
		return luaL_error(l, "bad second arg to cdobj.move");

	obj->offset = off;

	return 0;
}

static int ml_cdobj_size(lua_State* l) {
	checkargs(1, "cdobj.size");

	CDObj* obj = *checkcdobj(l, 1);

	if(obj->type == CD_CIRCLE) {
		lua_pushnumber(l, obj->size.radius);
		return 1;
	}
	if(obj->type == CD_AABB) {
		lua_pushnumber(l, obj->size.size.x);
		lua_pushnumber(l, obj->size.size.y);
		return 2;
	}

	return 0;
}

static int ml_cdobj_set_size(lua_State* l) {
	CDObj* obj = *checkcdobj(l, 1);

	int n = lua_gettop(l);
	if(n == 2) {
		assert(obj->type == CD_CIRCLE);
		float radius = luaL_checknumber(l, 2);
		obj->size.radius = radius;
		obj->dirty = true;
		return 0;
	}
	if(n == 3) {
		assert(obj->type == CD_AABB);
		float width = luaL_checknumber(l, 2);
		float height = luaL_checknumber(l, 3);
		obj->size.size.x = width;
		obj->size.size.y = height;
		obj->dirty = true;
		return 0;
	}
	
	return luaL_error(l, "bad args to cdobj.set_size");
}

static int ml_cdobj_rect(lua_State* l) {
	checkargs(1, "cdobj.rect");

	CDObj* obj = *checkcdobj(l, 1);

	RectF res;

	if(obj->type == CD_CIRCLE) {
		float half_radius = obj->size.radius / 2.0f;
		res = rectf(
			obj->pos.x - half_radius, obj->pos.y - half_radius,
			obj->pos.y + half_radius, obj->pos.y + half_radius
		);
	}
	else if(obj->type == CD_AABB) {
		res = rectf(
			obj->pos.x, obj->pos.y,
			obj->pos.x + obj->size.size.x, obj->pos.y + obj->size.size.y
		);
	}
    else {
        return 0;
    }

	_new_rect(l, res.left, res.top, res.right, res.bottom);
	return 1;
}

static int ml_cdobj_userdata(lua_State* l) {
	checkargs(1, "cdobj.userdata");

	CDObj* obj = *checkcdobj(l, 1);
	if(obj->userdata == NULL) {
		lua_pushnil(l);
		return 1;
	}

	int ref = (int)(size_t)(obj->userdata);
	lua_getref(l, ref);
	return 1;
}

static int ml_cdobj_set_userdata(lua_State* l) {
	checkargs(2, "cdobj.set_userdata");

	CDObj* obj = *checkcdobj(l, 1);
	if(obj->userdata != NULL) {
		int ref = (int)(size_t)(obj->userdata);
		lua_unref(l, ref);
	}

	if(!lua_isnil(l, 2)) {
		int ref = lua_ref(l, 1);
		obj->userdata = (void*)(size_t)ref; 
	}
	else {
		obj->userdata = NULL;
	}

	return 0;
}

static int ml_cdobj_mask(lua_State* l) {
	checkargs(1, "cdobj.mask");

	CDObj* obj = *checkcdobj(l, 1);

	lua_pushinteger(l, obj->mask);
	return 1;
}

static int ml_cdobj_set_mask(lua_State* l) {
	checkargs(2, "cdobj.set_mask");

	CDObj* obj = *checkcdobj(l, 1);
	uint mask = luaL_checkinteger(l, 2);

	obj->mask = mask;

	return 0;
}

static const luaL_Reg coldet_fun[] = {
	{"init", ml_coldet_init},
	{"close", ml_coldet_close},
	{"new_circle", ml_coldet_new_circle},
	{"new_aabb", ml_coldet_new_aabb},
	{"remove", ml_coldet_remove},
	{"query_circle", ml_coldet_query_circle},
	{"query_aabb", ml_coldet_query_aabb},
	{"cast_segment", ml_coldet_cast_segment},
	{"process", ml_coldet_process},
	{NULL, NULL}
};

static const luaL_Reg cdobj_fun[] = {
	{"is_circle", ml_cdobj_is_circle},
	{"is_aabb", ml_cdobj_is_aabb},
	{"pos", ml_cdobj_pos},
	{"set_pos", ml_cdobj_set_pos},
	{"move", ml_cdobj_move},
	{"size", ml_cdobj_size},
	{"set_size", ml_cdobj_set_size},
	{"rect", ml_cdobj_rect},
	{"userdata", ml_cdobj_userdata},
	{"set_userdata", ml_cdobj_set_userdata},
	{"mask", ml_cdobj_mask},
	{"set_mask", ml_cdobj_set_mask},
	{NULL, NULL}
};

int malka_open_coldet(lua_State* l) {
	luaL_newmetatable(l, "_CDWorld.mt");
	malka_register(l, "coldet", coldet_fun);

	luaL_newmetatable(l, "_CDObj.mt");
	malka_register(l, "cdobj", cdobj_fun);

	lua_pop(l, 2);

	return 1;
}
