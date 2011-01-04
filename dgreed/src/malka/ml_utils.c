#include "ml_utils.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>

// 2d vectors

static void _new_vec2(lua_State* l, double x, double y) {
	lua_createtable(l, 0, 2);
	int table = lua_gettop(l);
	lua_pushnumber(l, y);
	lua_pushnumber(l, x);
	lua_setfield(l, table, "x");
	lua_setfield(l, table, "y");
	luaL_getmetatable(l, "_vec2.mt");
	lua_setmetatable(l, -2);
}

static int ml_vec2(lua_State* l) {
	int n = lua_gettop(l);
	if(n == 0) {
		_new_vec2(l, 0.0, 0.0);
	}
	else if(n == 2) {
		double x = luaL_checknumber(l, 1);
		double y = luaL_checknumber(l, 2);
		_new_vec2(l, x, y);
	}
	else
		return luaL_error(l, "wrong number of arguments provided to vec2");
	return 1;	
}

static int ml_dot(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to dot");
	
	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");

	double x1 = luaL_checknumber(l, 3);
	double y1 = luaL_checknumber(l, 4);
	double x2 = luaL_checknumber(l, 5);
	double y2 = luaL_checknumber(l, 6);

	lua_pushnumber(l, x1*x2 + y1*y2);
	return 1;
}

static int ml_length(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to length");

	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 2);
	double y = luaL_checknumber(l, 3);

	lua_pushnumber(l, sqrt(x*x + y*y));
	return 1;
}

static int ml_length_sq(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to length_sq");

	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 2);
	double y = luaL_checknumber(l, 3);

	lua_pushnumber(l, x*x + y*y);
	return 1;
}

static int ml_normalize(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to length_sq");

	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 2);
	double y = luaL_checknumber(l, 3);
	double inv_len = 1.0 / sqrt(x*x + y*y);

	_new_vec2(l, x * inv_len, y * inv_len);
	return 1;
}

static int ml_add(lua_State* l) {
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");

	double x1 = luaL_checknumber(l, 3);
	double y1 = luaL_checknumber(l, 4);
	double x2 = luaL_checknumber(l, 5);
	double y2 = luaL_checknumber(l, 6);

	_new_vec2(l, x1+x2, y1+y2);
	return 1;
}

static int ml_sub(lua_State* l) {
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");

	double x1 = luaL_checknumber(l, 3);
	double y1 = luaL_checknumber(l, 4);
	double x2 = luaL_checknumber(l, 5);
	double y2 = luaL_checknumber(l, 6);

	_new_vec2(l, x1-x2, y1-y2);
	return 1;
}

static int ml_mul(lua_State* l) {
	double s = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 3);
	double y = luaL_checknumber(l, 4);

	_new_vec2(l, x*s, y*s);
	return 1;
}

static int ml_div(lua_State* l) {
	double s = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 3);
	double y = luaL_checknumber(l, 4);

	_new_vec2(l, x/s, y/s);
	return 1;
}

static int ml_negate(lua_State* l) {
	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	// Why the hell 3, 4 works and 2, 3 doesn't?
	double x = luaL_checknumber(l, 3);
	double y = luaL_checknumber(l, 4);

	_new_vec2(l, -x, -y);
	return 1;
}

static int ml_vec2_tostring(lua_State* l) {
	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 2);
	double y = luaL_checknumber(l, 3);
	lua_pushfstring(l, "vec2(%f, %f)", x, y);
	return 1;
}

static const luaL_Reg vec2_fun[] = {
	{"vec2", ml_vec2},
	{"dot", ml_dot},
	{"length", ml_length},
	{"length_sq", ml_length_sq},
	{"normalize", ml_normalize},
	{NULL, NULL}
};

static const luaL_Reg vec2_mt[] = {
	{"__add", ml_add},
	{"__sub", ml_sub},
	{"__mul", ml_mul},
	{"__div", ml_div},
	{"__unm", ml_negate},
	{"__tostring", ml_vec2_tostring},
	{NULL, NULL}
};

int malka_open_vec2(lua_State* l) {
	// metatable
	luaL_newmetatable(l, "_vec2.mt");
	//lua_pushvalue(l, -1);
	//lua_setfield(l, -2, "__index");
	luaL_register(l, NULL, vec2_mt);

	// functions
	// register manually, to appear globally
	int i = 0;
	while(vec2_fun[i].name) {
		lua_register(l, vec2_fun[i].name, vec2_fun[i].func);
		i++;
	}

	return 1;
}

// rectangle

static void _new_rect(lua_State* l, double _l, double t,
	double r, double b) {
	lua_createtable(l, 0, 4);
	int table = lua_gettop(l);
	lua_pushnumber(l, b);
	lua_pushnumber(l, r);
	lua_pushnumber(l, t);
	lua_pushnumber(l, _l);
	lua_setfield(l, table, "l");
	lua_setfield(l, table, "t");
	lua_setfield(l, table, "r");
	lua_setfield(l, table, "b");
	luaL_getmetatable(l, "_rect.mt");
	lua_setmetatable(l, -2);
}

static int ml_rect(lua_State* l) {
	int n = lua_gettop(l);
	if(n == 0) {
		_new_rect(l, 0.0, 0.0, 0.0, 0.0);
	}
	else if(n == 2) {
		double _l = luaL_checknumber(l, 1);
		double t = luaL_checknumber(l, 2);
		_new_rect(l, _l, t, 0.0, 0.0);
	}
	else if(n == 4) {
		double _l = luaL_checknumber(l, 1);
		double t = luaL_checknumber(l, 2);
		double r = luaL_checknumber(l, 3);
		double b = luaL_checknumber(l, 4);
		_new_rect(l, _l, t, r, b);
	}
	else
		return luaL_error(l, "wrong number of arguments provided to rect");
	return 1;	
}

static int ml_width(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to width");

	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "r");

	double _l = luaL_checknumber(l, 2);
	double r = luaL_checknumber(l, 3);

	lua_pushnumber(l, r - _l);

	return 1;
}

static int ml_height(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to height");

	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "b");

	double t = luaL_checknumber(l, 2);
	double b = luaL_checknumber(l, 3);

	lua_pushnumber(l, b - t);

	return 1;
}

static int ml_rect_rect_collision(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to rect_rect_collision");
	
	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 2, "l");
	lua_getfield(l, 2, "t");
	lua_getfield(l, 2, "r");
	lua_getfield(l, 2, "b");

	RectF rect1 = {
		luaL_checknumber(l, 3),
		luaL_checknumber(l, 4),
		luaL_checknumber(l, 5),
		luaL_checknumber(l, 6)
	};

	RectF rect2 = {
		luaL_checknumber(l, 7),
		luaL_checknumber(l, 8),
		luaL_checknumber(l, 9),
		luaL_checknumber(l, 10)
	};

	lua_pushboolean(l, rectf_rectf_collision(&rect1, &rect2));
	return 1;
}

static int ml_rect_point_collision(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to rect_point_collision");
	
	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");

	RectF rect = {
		luaL_checknumber(l, 3),
		luaL_checknumber(l, 4),
		luaL_checknumber(l, 5),
		luaL_checknumber(l, 6)
	};

	Vector2 point = {
		luaL_checknumber(l, 7),
		luaL_checknumber(l, 8)
	};

	lua_pushboolean(l, rectf_contains_point(&rect, &point));
	return 1;
}

static int ml_rect_circle_collision(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to rect_circle_collision");
	
	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");

	float r = luaL_checknumber(l, 3);

	RectF rect = {
		luaL_checknumber(l, 4),
		luaL_checknumber(l, 5),
		luaL_checknumber(l, 6),
		luaL_checknumber(l, 7)
	};

	Vector2 point = {
		luaL_checknumber(l, 8),
		luaL_checknumber(l, 9)
	};

	lua_pushboolean(l, rectf_circle_collision(&rect, &point, r));
	return 1;
}

static int ml_rect_tostring(lua_State* l) {
	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "b");

	double _l = luaL_checknumber(l, 2);
	double t = luaL_checknumber(l, 3);
	double r = luaL_checknumber(l, 4);
	double b = luaL_checknumber(l, 5);

	lua_pushfstring(l, "rect(%f, %f, %f, %f)", _l, t, r, b);
	return 1;
}

static const luaL_Reg rect_fun[] = {
	{"rect", ml_rect},
	{"width", ml_width},
	{"height", ml_height},
	{"rect_rect_collision", ml_rect_rect_collision},
	{"rect_point_collision", ml_rect_point_collision},
	{"rect_circle_collision", ml_rect_circle_collision},
	{NULL, NULL}
};

static const luaL_Reg rect_mt[] = {
	{"__tostring", ml_rect_tostring},
	{NULL, NULL}
};	

int malka_open_rect(lua_State* l) {
	// metatable
	luaL_newmetatable(l, "_rect.mt");
	luaL_register(l, NULL, rect_mt);

	//functions
	int i = 0;
	while(rect_fun[i].name) {
		lua_register(l, rect_fun[i].name, rect_fun[i].func);
		i++;
	}

	return 1;
}

