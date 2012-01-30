#include "ml_utils.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <memory.h>
#include <time.h>

// 2d vectors

void _new_vec2(lua_State* l, double x, double y) {
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
	else if(n == 1) {
		luaL_checktype(l, 1, LUA_TTABLE);
		lua_getfield(l, 1, "x");
		lua_getfield(l, 1, "y");
		double x = luaL_checknumber(l, 2);
		double y = luaL_checknumber(l, 3);
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

static int ml_rotate(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to rotate");

	luaL_checktype(l, 1, LUA_TTABLE);
	double angle = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");

	double x = luaL_checknumber(l, 3);
	double y = luaL_checknumber(l, 4);
	double s = sin(angle);
	double c = cos(angle);

	_new_vec2(l, c*x - s*y, s*x + c*y);
	return 1;
}

static int ml_add(lua_State* l) {
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

	_new_vec2(l, x1+x2, y1+y2);
	return 1;
}

static int ml_sub(lua_State* l) {
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
	{"rotate", ml_rotate},
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

void _new_rect(lua_State* l, double _l, double t,
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
	else if(n == 1) {
		luaL_checktype(l, 1, LUA_TTABLE);
		lua_getfield(l, 1, "l");
		lua_getfield(l, 1, "t");
		lua_getfield(l, 1, "r");
		lua_getfield(l, 1, "b");
		double _l = luaL_checknumber(l, 2);
		double t = luaL_checknumber(l, 3);
		double r = luaL_checknumber(l, 4);
		double b = luaL_checknumber(l, 5);
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
	if(n != 3)
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

static int ml_rect_tri_collision(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 4)
		return luaL_error(l, "wrong number of arguments provided to rect_tri_collision");
	
	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);
	luaL_checktype(l, 3, LUA_TTABLE);
	luaL_checktype(l, 4, LUA_TTABLE);

	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");
	lua_getfield(l, 3, "x");
	lua_getfield(l, 3, "y");
	lua_getfield(l, 4, "x");
	lua_getfield(l, 4, "y");

	RectF rect = {
		luaL_checknumber(l, 5),
		luaL_checknumber(l, 6),
		luaL_checknumber(l, 7),
		luaL_checknumber(l, 8)
	};

	Vector2 a = {
		luaL_checknumber(l, 9),
		luaL_checknumber(l, 10)
	};

	Vector2 b = {
		luaL_checknumber(l, 11),
		luaL_checknumber(l, 12)
	};

	Vector2 c = {
		luaL_checknumber(l, 13),
		luaL_checknumber(l, 14)
	};

	Triangle tri = {a, b, c};
	lua_pushboolean(l, tri_rectf_collision(&tri, &rect));
	return 1;
}

static int ml_rect_raycast(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided to rect_raycast");

	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);
	luaL_checktype(l, 3, LUA_TTABLE);

	lua_getfield(l, 1, "l");
	lua_getfield(l, 1, "t");
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");
	lua_getfield(l, 3, "x");
	lua_getfield(l, 3, "y");

	RectF rect = {
		luaL_checknumber(l, 4),
		luaL_checknumber(l, 5),
		luaL_checknumber(l, 6),
		luaL_checknumber(l, 7)
	};

	Vector2 start = {
		luaL_checknumber(l, 8),
		luaL_checknumber(l, 9)
	};

	Vector2 end = {
		luaL_checknumber(l, 10),
		luaL_checknumber(l, 11)
	};

	Vector2 hit = rectf_raycast(&rect, &start, &end);	

	_new_vec2(l, hit.x, hit.y);

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

static int ml_segment_intersect(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 4)
		return luaL_error(l, "wrong number of arguments provided to segment_intersect");
	
	luaL_checktype(l, 1, LUA_TTABLE);
	luaL_checktype(l, 2, LUA_TTABLE);
	luaL_checktype(l, 3, LUA_TTABLE);
	luaL_checktype(l, 4, LUA_TTABLE);
	
	lua_getfield(l, 1, "x");
	lua_getfield(l, 1, "y");
	lua_getfield(l, 2, "x");
	lua_getfield(l, 2, "y");
	lua_getfield(l, 3, "x");
	lua_getfield(l, 3, "y");
	lua_getfield(l, 4, "x");
	lua_getfield(l, 4, "y");

	Vector2 a1 = {
		luaL_checknumber(l, 5),
		luaL_checknumber(l, 6)
	};

	Vector2 a2 = {
		luaL_checknumber(l, 7),
		luaL_checknumber(l, 8)
	};

	Vector2 b1 = {
		luaL_checknumber(l, 9),
		luaL_checknumber(l, 10)
	};

	Vector2 b2 = {
		luaL_checknumber(l, 11),
		luaL_checknumber(l, 12)
	};

	Segment seg1 = {a1, a2};
	Segment seg2 = {b1, b2};
	Vector2 hit;

	if(segment_intersect(seg1, seg2, &hit)) 
		_new_vec2(l, hit.x, hit.y);
	else
		lua_pushnil(l);

	return 1;
}

static const luaL_Reg rect_fun[] = {
	{"rect", ml_rect},
	{"width", ml_width},
	{"height", ml_height},
	{"rect_rect_collision", ml_rect_rect_collision},
	{"rect_point_collision", ml_rect_point_collision},
	{"rect_circle_collision", ml_rect_circle_collision},
	{"rect_tri_collision", ml_rect_tri_collision},
	{"rect_raycast", ml_rect_raycast},
	{"segment_intersect", ml_segment_intersect},
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


// colors

void _new_rgba(lua_State* l, double r, double g,
	double b, double a) {
	lua_createtable(l, 0, 4);
	int table = lua_gettop(l);
	lua_pushnumber(l, a);
	lua_pushnumber(l, b);
	lua_pushnumber(l, g);
	lua_pushnumber(l, r);
	lua_setfield(l, table, "r");
	lua_setfield(l, table, "g");
	lua_setfield(l, table, "b");
	lua_setfield(l, table, "a");
	luaL_getmetatable(l, "_color_rgba.mt");
	lua_setmetatable(l, -2);
}

static void _new_hsva(lua_State* l, double h, double s,
	double v, double a) {
	lua_createtable(l, 0, 4);
	int table = lua_gettop(l);
	lua_pushnumber(l, a);
	lua_pushnumber(l, v);
	lua_pushnumber(l, s);
	lua_pushnumber(l, h);
	lua_setfield(l, table, "h");
	lua_setfield(l, table, "s");
	lua_setfield(l, table, "v");
	lua_setfield(l, table, "a");
	luaL_getmetatable(l, "_color_hsva.mt");
	lua_setmetatable(l, -2);
}

static int ml_rgba(lua_State* l) {
	int n = lua_gettop(l);
	double r, g, b, a;
	if(n == 3) {
		r = luaL_checknumber(l, 1);
		g = luaL_checknumber(l, 2);
		b = luaL_checknumber(l, 3);
		a = 1.0;
		_new_rgba(l, r, g, b, a);
	}
	else if(n == 4) {
		r = luaL_checknumber(l, 1);
		g = luaL_checknumber(l, 2);
		b = luaL_checknumber(l, 3);
		a = luaL_checknumber(l, 4);
		_new_rgba(l, r, g, b, a);
	}
	else
		return luaL_error(l, "wrong number of arguments provided to rgba");
	return 1;
}

// Following are higher precision versions of hsv/rgb conversion than
// in utils.h, duplication is justified

static void _hsv_to_rgb(double h, double s, double v,
	double* r, double* g, double* b) {
	if(s == 0.0) {
		*r = *g = *b = v;
		return;
	}	
	
	h *= 6.0;
	uint i = (uint)floor(h);
	double f = h - (double)i;

	double p = v * (1.0 - s);
	double q = v * (1.0 - (s * f));
	double t = v * (1.0 - (s * (1.0 - f)));

	switch(i) {
		case 6:
		case 0:
			*r = v; *g = t; *b = p;
			return;
		case 1:
			*r = q; *g = v; *b = p;
			return;
		case 2:
			*r = p; *g = v; *b = t;
			return;
		case 3:
			*r = p; *g = q; *b = v;
			return;
		case 4:
			*r = t; *g = p; *b = v;
			return;
		case 5:
			*r = v; *g = p; *b = q;
			return;
		default:
			*r = *g = *b = 0.0;
			return;
	}
}

void _rgb_to_hsv(double r, double g, double b,
	double* h, double* s, double* v) {
	*h = *s = *v = 0.0;

	double min = MIN(r, MIN(g, b));
	double max = MAX(r, MAX(g, b));
	*v = max;
	double delta = max - min;

	if(max != 0.0)
		*s = delta / max;
	else
		return;

	if(r == max)
		*h = (g - b) / delta;
	else if(g == max)
		*h = 2.0 + (b - r) / delta;
	else
		*h = 4.0 + (r - g) / delta;
	*h /= 6.0;
	if(*h < 0.0)
		*h += 1.0;
}

static int ml_hsva(lua_State* l) {
	int n = lua_gettop(l);
	double h, s, v, a;
	if(n == 3) {
		h = luaL_checknumber(l, 1);
		s = luaL_checknumber(l, 2);
		v = luaL_checknumber(l, 3);
		a = 1.0;
		_new_hsva(l, h, s, v, a);
	}
	else if(n == 4) {
		h = luaL_checknumber(l, 1);
		s = luaL_checknumber(l, 2);
		v = luaL_checknumber(l, 3);
		a = luaL_checknumber(l, 4);
		_new_hsva(l, h, s, v, a);
	}
	else
		return luaL_error(l, "wrong number of arguments provided to hsva");
	return 1;
}

static int ml_to_rgba(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to to_rgba");
	
	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "h");
	lua_getfield(l, 1, "s");
	lua_getfield(l, 1, "v");
	lua_getfield(l, 1, "a");

	double h = luaL_checknumber(l, 2);
	double s = luaL_checknumber(l, 3);
	double v = luaL_checknumber(l, 4);
	double a = luaL_checknumber(l, 5);

	double r, g, b;

	_hsv_to_rgb(h, s, v, &r, &g, &b);
	_new_rgba(l, r, g, b, a);

	return 1;
}

static int ml_to_hsva(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to to_rgba");
	
	luaL_checktype(l, 1, LUA_TTABLE);

	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "g");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 1, "a");

	double r = luaL_checknumber(l, 2);
	double g = luaL_checknumber(l, 3);
	double b = luaL_checknumber(l, 4);
	double a = luaL_checknumber(l, 5);

	double h, s, v;

	_rgb_to_hsv(r, g, b, &h, &s, &v);
	_new_hsva(l, h, s, v, a);

	return 1;
}

static int ml_color_add_rgba(lua_State* l) {
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "g");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 1, "a");
	lua_getfield(l, 2, "r");
	lua_getfield(l, 2, "g");
	lua_getfield(l, 2, "b");
	lua_getfield(l, 2, "a");

	double r = luaL_checknumber(l, 3) + luaL_checknumber(l, 7);
	double g = luaL_checknumber(l, 4) + luaL_checknumber(l, 8);
	double b = luaL_checknumber(l, 5) + luaL_checknumber(l, 9);
	double a = luaL_checknumber(l, 6) + luaL_checknumber(l, 10);

	_new_rgba(l, r, g, b, a);
	return 1;
}

static int ml_color_add_hsva(lua_State* l) {
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "h");
	lua_getfield(l, 1, "s");
	lua_getfield(l, 1, "v");
	lua_getfield(l, 1, "a");
	lua_getfield(l, 2, "h");
	lua_getfield(l, 2, "s");
	lua_getfield(l, 2, "v");
	lua_getfield(l, 2, "a");

	double h = luaL_checknumber(l, 3) + luaL_checknumber(l, 7);
	double s = luaL_checknumber(l, 4) + luaL_checknumber(l, 8);
	double v = luaL_checknumber(l, 5) + luaL_checknumber(l, 9);
	double a = luaL_checknumber(l, 6) + luaL_checknumber(l, 10);

	_new_hsva(l, h, s, v, a);
	return 1;
}

static int ml_color_sub_rgba(lua_State* l) {
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "g");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 1, "a");
	lua_getfield(l, 2, "r");
	lua_getfield(l, 2, "g");
	lua_getfield(l, 2, "b");
	lua_getfield(l, 2, "a");

	double r = luaL_checknumber(l, 3) - luaL_checknumber(l, 7);
	double g = luaL_checknumber(l, 4) - luaL_checknumber(l, 8);
	double b = luaL_checknumber(l, 5) - luaL_checknumber(l, 9);
	double a = luaL_checknumber(l, 6) - luaL_checknumber(l, 10);

	_new_rgba(l, r, g, b, a);
	return 1;
}

static int ml_color_sub_hsva(lua_State* l) {
	luaL_checktype(l, 2, LUA_TTABLE);

	lua_getfield(l, 1, "h");
	lua_getfield(l, 1, "s");
	lua_getfield(l, 1, "v");
	lua_getfield(l, 1, "a");
	lua_getfield(l, 2, "h");
	lua_getfield(l, 2, "s");
	lua_getfield(l, 2, "v");
	lua_getfield(l, 2, "a");

	double h = luaL_checknumber(l, 3) - luaL_checknumber(l, 7);
	double s = luaL_checknumber(l, 4) - luaL_checknumber(l, 8);
	double v = luaL_checknumber(l, 5) - luaL_checknumber(l, 9);
	double a = luaL_checknumber(l, 6) - luaL_checknumber(l, 10);

	_new_hsva(l, h, s, v, a);
	return 1;
}

static int ml_color_mul_rgba(lua_State* l) {
	double k = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "g");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 1, "a");

	double r = luaL_checknumber(l, 3) * k;
	double g = luaL_checknumber(l, 4) * k;
	double b = luaL_checknumber(l, 5) * k;
	double a = luaL_checknumber(l, 6) * k;

	_new_rgba(l, r, g, b, a);
	return 1;
}

static int ml_color_mul_hsva(lua_State* l) {
	double k = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "h");
	lua_getfield(l, 1, "s");
	lua_getfield(l, 1, "v");
	lua_getfield(l, 1, "a");

	double h = luaL_checknumber(l, 3) * k;
	double s = luaL_checknumber(l, 4) * k;
	double v = luaL_checknumber(l, 5) * k;
	double a = luaL_checknumber(l, 6) * k;

	_new_hsva(l, h, s, v, a);
	return 1;
}

static int ml_color_div_rgba(lua_State* l) {
	double k = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "g");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 1, "a");

	double r = luaL_checknumber(l, 3) / k;
	double g = luaL_checknumber(l, 4) / k;
	double b = luaL_checknumber(l, 5) / k;
	double a = luaL_checknumber(l, 6) / k;

	_new_rgba(l, r, g, b, a);
	return 1;
}

static int ml_color_div_hsva(lua_State* l) {
	double k = luaL_checknumber(l, 2);

	lua_getfield(l, 1, "h");
	lua_getfield(l, 1, "s");
	lua_getfield(l, 1, "v");
	lua_getfield(l, 1, "a");

	double h = luaL_checknumber(l, 3) / k;
	double s = luaL_checknumber(l, 4) / k;
	double v = luaL_checknumber(l, 5) / k;
	double a = luaL_checknumber(l, 6) / k;

	_new_hsva(l, h, s, v, a);
	return 1;
}

static int ml_color_tostring_rgba(lua_State* l) {
	lua_getfield(l, 1, "r");
	lua_getfield(l, 1, "g");
	lua_getfield(l, 1, "b");
	lua_getfield(l, 1, "a");

	double r = luaL_checknumber(l, 2);
	double g = luaL_checknumber(l, 3);
	double b = luaL_checknumber(l, 4);
	double a = luaL_checknumber(l, 5);

	lua_pushfstring(l, "rgba(%f, %f, %f, %f)", r, g, b, a);
	return 1;
}

static int ml_color_tostring_hsva(lua_State* l) {
	lua_getfield(l, 1, "h");
	lua_getfield(l, 1, "s");
	lua_getfield(l, 1, "v");
	lua_getfield(l, 1, "a");

	double h = luaL_checknumber(l, 2);
	double s = luaL_checknumber(l, 3);
	double v = luaL_checknumber(l, 4);
	double a = luaL_checknumber(l, 5);

	lua_pushfstring(l, "hsva(%f, %f, %f, %f)", h, s, v, a);
	return 1;
}

static const luaL_Reg colors_fun[] = {
	{"rgba", ml_rgba},
	{"hsva", ml_hsva},
	{"to_rgba", ml_to_rgba},
	{"to_hsva", ml_to_hsva},
	{NULL, NULL}
};	

static const luaL_Reg color_rgba_mt[] = {
	{"__add", ml_color_add_rgba},
	{"__sub", ml_color_sub_rgba},
	{"__mul", ml_color_mul_rgba},
	{"__div", ml_color_div_rgba},
	{"__tostring", ml_color_tostring_rgba},
	{NULL, NULL}
};	

static const luaL_Reg color_hsva_mt[] = {
	{"__add", ml_color_add_hsva},
	{"__sub", ml_color_sub_hsva},
	{"__mul", ml_color_mul_hsva},
	{"__div", ml_color_div_hsva},
	{"__tostring", ml_color_tostring_hsva},
	{NULL, NULL}
};

int malka_open_colors(lua_State* l) {
	// metatables
	luaL_newmetatable(l, "_color_rgba.mt");
	luaL_register(l, NULL, color_rgba_mt);
	luaL_newmetatable(l, "_color_hsva.mt");
	luaL_register(l, NULL, color_hsva_mt);

	// functions
	int i = 0;
	while(colors_fun[i].name) {
		lua_register(l, colors_fun[i].name, colors_fun[i].func);
		i++;
	}

	return 1;
}

// misc

double _lerp(double a, double b, double t) {
	return a + (b-a)*t;
}

// This is the most evil lerp I've ever seen!
static int _ml_lerp_internal(lua_State* l, double t) {
	if(lua_isnumber(l, 1) && lua_isnumber(l, 2)) {
		double a = lua_tonumber(l, 1);
		double b = lua_tonumber(l, 2);
		lua_pushnumber(l, _lerp(a, b, t)); 
		return 1;
	}
	else {
		if(lua_istable(l, 1) && lua_istable(l, 2)) {
			lua_getfield(l, 1, "x");
			lua_getfield(l, 1, "y");
			lua_getfield(l, 2, "x");
			lua_getfield(l, 2, "y");

			for(int i = -1; i >= -4; --i) {
				if(lua_isnil(l, i)) {
					lua_pop(l, 4);
					goto color_rgba;
				}
			}	

			double by = lua_tonumber(l, -1);
			double bx = lua_tonumber(l, -2);
			double ay = lua_tonumber(l, -3);
			double ax = lua_tonumber(l, -4);

			_new_vec2(l, 
				_lerp(ax, bx, t),
				_lerp(ay, by, t)
			);	
			return 1;
			
		color_rgba:
			lua_getfield(l, 1, "r");
			lua_getfield(l, 1, "g");
			lua_getfield(l, 1, "b");
			lua_getfield(l, 1, "a");
			lua_getfield(l, 2, "r");
			lua_getfield(l, 2, "g");
			lua_getfield(l, 2, "b");
			lua_getfield(l, 2, "a");

			for(int i = -1; i >= -8; --i) {
				if(lua_isnil(l, i)) {
					lua_pop(l, 8);
					goto color_hsva;
				}
			}	

			double ba = lua_tonumber(l, -1);
			double bb = lua_tonumber(l, -2);
			double bg = lua_tonumber(l, -3);
			double br = lua_tonumber(l, -4);
			double aa = lua_tonumber(l, -5);
			double ab = lua_tonumber(l, -6);
			double ag = lua_tonumber(l, -7);
			double ar = lua_tonumber(l, -8);

			_new_rgba(l, 
				_lerp(ar, br, t),
				_lerp(ag, bg, t),
				_lerp(ab, bb, t),
				_lerp(aa, ba, t)
			);
			return 1;

		color_hsva:
			lua_getfield(l, 1, "h");
			lua_getfield(l, 1, "s");
			lua_getfield(l, 1, "v");
			lua_getfield(l, 1, "a");
			lua_getfield(l, 2, "h");
			lua_getfield(l, 2, "s");
			lua_getfield(l, 2, "v");
			lua_getfield(l, 2, "a");

			for(int i = -1; i >= -8; --i) {
				if(lua_isnil(l, i)) {
					lua_pop(l, 8);
					goto error;
				}
			}	

			/*double*/ ba = lua_tonumber(l, -1);
			double bv = lua_tonumber(l, -2);
			double bs = lua_tonumber(l, -3);
			double bh = lua_tonumber(l, -4);
			/*double*/ aa = lua_tonumber(l, -5);
			double av = lua_tonumber(l, -6);
			double as = lua_tonumber(l, -7);
			double ah = lua_tonumber(l, -8);

			_new_hsva(l, 
				_lerp(ah, bh, t),
				_lerp(as, bs, t),
				_lerp(av, bv, t),
				_lerp(aa, ba, t)
			);
			return 1;
		}
	}
error:
	return luaL_error(l, "bad arguments provided for lerp/smoothstep");
}

static int ml_lerp(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided to lerp");

	double t = luaL_checknumber(l, 3);
	return _ml_lerp_internal(l, t);
}	

static int ml_smoothstep(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided for smoothstep");

	double t = luaL_checknumber(l, 3);
	t = MIN(1.0, MAX(0.0, t));
	t = t * t * (3.0 - 2.0 * t);
	return _ml_lerp_internal(l, t);
}

static int ml_clamp(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided for clamp");

	double a = luaL_checknumber(l, 1);	
	double b = luaL_checknumber(l, 2);	
	double t = luaL_checknumber(l, 3);	

	if(b <= a) 
		return luaL_error(l, "bad range provided for clamp");

	if(t < a) t = a;
	if(t > b) t = b;

	lua_pushnumber(l, t);
	return 1;
}

static int ml_hash(lua_State* l) {
	int n = lua_gettop(l);
	uint seed = 0;
	if(n == 2) {
		seed = luaL_checkinteger(l, 2);
		n--;
	}

	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to hash");

	size_t len;
	const char* str = luaL_checklstring(l, 1, &len);
	uint hash = hash_murmur(str, len, seed);

	lua_pushinteger(l, hash);
	return 1;
}

static const luaL_Reg misc_fun[] = {
	{"lerp", ml_lerp},
	{"smoothstep", ml_smoothstep},
	{"clamp", ml_clamp},
	{"hash", ml_hash},
	{NULL, NULL}
};

int malka_open_misc(lua_State* l) {
	int i = 0;
	while(misc_fun[i].name) {
		lua_register(l, misc_fun[i].name, misc_fun[i].func);
		i++;
	}
	return 1;
}


// rand

static int ml_rand_seed(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to rand.seed");
	
	uint seed = luaL_checkinteger(l, 1);
	rand_init(seed);
	return 0;
}

static int ml_rand_int(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to rand.int");

	int min = luaL_checkinteger(l, 1);
	int max = luaL_checkinteger(l, 2);

	lua_pushinteger(l, rand_int(min, max));
	return 1;
}

static int ml_rand_float(lua_State* l) {
	int n = lua_gettop(l);
	double min = 0.0, max = 1.0;
	if(n == 2) {
		min = luaL_checknumber(l, 1);
		max = luaL_checknumber(l, 2);
		n = 0;
	}

	if(n != 0)
		return luaL_error(l, "wrong number of arguments provided to rand.float");

	double r = (double)rand_uint() / (double)(MAX_UINT32+1LL);
	r = min + r * (max - min);
	lua_pushnumber(l, r);
	return 1;
}

static const luaL_Reg rand_fun[] = {
	{"seed", ml_rand_seed},
	{"int", ml_rand_int},
	{"float", ml_rand_float},
	{NULL, NULL}
};

int malka_open_rand(lua_State* l) {
	rand_init(time(NULL));

	luaL_register(l, "rand", rand_fun);
	return 1;
}


// log

static int ml_log_error(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to log.error");
	
	const char* msg = luaL_checkstring(l, 1);
	LOG_ERROR(msg);
	return 0;
}

static int ml_log_warning(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to log.warning");
	
	const char* msg = luaL_checkstring(l, 1);
	LOG_WARNING(msg);
	return 0;
}

static int ml_log_info(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to log.info");
	
	const char* msg = luaL_checkstring(l, 1);
	LOG_INFO(msg);
	return 0;
}

static const luaL_Reg log_fun[] = {
	{"error", ml_log_error},
	{"warning", ml_log_warning},
	{"info", ml_log_info},
	{NULL, NULL}
};	

int malka_open_log(lua_State* l) {
	luaL_register(l, "log", log_fun);
	return 1;
}


// file

static int ml_file_exists(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to file.exists");

	const char* name = luaL_checkstring(l, 1);
	lua_pushboolean(l, file_exists(name));
	return 1;
}

static int ml_file_read(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to file.read");

	const char* name = luaL_checkstring(l, 1);
	const char* contents = txtfile_read(name);
	if(contents) {
		lua_pushstring(l, contents);
		MEM_FREE(contents);
		return 1;
	}

	lua_pushnil(l);
	return 1;
}

static int ml_file_write(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to file.write");

	const char* name = luaL_checkstring(l, 1);
	const char* data = luaL_checkstring(l, 2);
	txtfile_write(name, data);

	return 0;
}

static const luaL_Reg file_fun[] = {
	{"exists", ml_file_exists},
	{"read", ml_file_read},
	{"write", ml_file_write},
	{NULL, NULL}
};	

int malka_open_file(lua_State* l) {
	luaL_register(l, "file", file_fun);
	return 1;
}

