#include "ml_system.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <memory.h>
#include <system.h>
#include <font.h>
#include <gfx_utils.h>

extern void _new_vec2(lua_State* l, double x, double y);
extern void _new_rect(lua_State* l, double _l, double t,
	double r, double b);

// time

static int ml_time_ms(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 0)
		return luaL_error(l, "time.ms() does not take any arguments");

	lua_pushnumber(l, time_ms());
	return 1;
}

static int ml_time_s(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 0)
		return luaL_error(l, "time.s() does not take any arguments");

	lua_pushnumber(l, (double)time_ms() / 1000.0);
	return 1;
}

static int ml_time_dt(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 0)
		return luaL_error(l, "time.dt() does not take any arguments");

	lua_pushnumber(l, time_delta());
	return 1;
}

static int ml_time_fps(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 0)
		return luaL_error(l, "time.fps() does not take any arguments");

	lua_pushinteger(l, time_fps());
	return 1;
}

static const luaL_Reg time_fun[] = {
	{"ms", ml_time_ms},
	{"s", ml_time_s},
	{"dt", ml_time_dt},
	{"fps", ml_time_fps},
	{NULL, NULL}
};	


// texture

#define checktexhandle(l, i) \
	(TexHandle*)luaL_checkudata(l, i, "_TexHandle.mt")

static void _new_texhandle(lua_State* l, TexHandle h) {
	TexHandle* t = (TexHandle*)lua_newuserdata(l, sizeof(TexHandle));
	*t = h;
	luaL_getmetatable(l, "_TexHandle.mt");
	lua_setmetatable(l, -2);
}

static int ml_tex_load(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to tex.load");

	const char* filename = luaL_checkstring(l, 1);
	TexHandle h = tex_load(filename);
	_new_texhandle(l, h);
	return 1;
}

static int ml_tex_size(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to tex.size");

	TexHandle* t = checktexhandle(l, 1);
	uint w, h;
	tex_size(*t, &w, &h);
	lua_pushinteger(l, w);
	lua_pushinteger(l, h);
	return 2;
}

static int ml_tex_free(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to tex.free");

	TexHandle* h = checktexhandle(l, 1);
	tex_free(*h);
	return 0;
}

static const luaL_Reg tex_fun[] = {
	{"load", ml_tex_load},
	{"size", ml_tex_size},
	{"free", ml_tex_free},
	{NULL, NULL}
};	


// font

#define checkfonthandle(l, i) \
	(TexHandle*)luaL_checkudata(l, i, "_FontHandle.mt")

static void _new_fonthandle(lua_State* l, FontHandle h) {
	FontHandle* t = (FontHandle*)lua_newuserdata(l, sizeof(FontHandle));
	*t = h;
	luaL_getmetatable(l, "_FontHandle.mt");
	lua_setmetatable(l, -2);
}

static int ml_font_load(lua_State* l) {
	int n = lua_gettop(l);
	double scale = 1.0;
	if(n == 2) {
		scale = luaL_checknumber(l, 2);
		n--;
	}

	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to font.load");

	const char* filename = luaL_checkstring(l, 1);
	FontHandle h = font_load_ex(filename, (float)scale);
	_new_fonthandle(l, h);
	return 1;
}

static int ml_font_size(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to font.size");

	FontHandle* h = checkfonthandle(l, 1);
	const char* text = luaL_checkstring(l, 2);

	lua_pushnumber(l, font_width(*h, text));
	lua_pushnumber(l, font_height(*h));
	return 2;
}

static int ml_font_rect(lua_State* l) {
	int n = lua_gettop(l);
	double scale = 1.0;
	if(n == 4) {
		scale = luaL_checknumber(l, 4);
		n--;
	}

	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided to font.rect");

	FontHandle* h = checkfonthandle(l, 1);
	const char* text = luaL_checkstring(l, 2);

	lua_getfield(l, 3, "x");
	lua_getfield(l, 3, "y");

	double x = luaL_checknumber(l, 4);
	double y = luaL_checknumber(l, 5);

	Vector2 p = vec2((float)x, (float)y);
	RectF r = font_rect_ex(*h, text, &p, (float)scale);

	_new_rect(l, (float)r.left, (float)r.top, 
		(float)r.right, (float)r.bottom);

	return 1;
}

static int ml_font_free(lua_State* l) {
	int n = lua_gettop(l);
	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to font.free");

	FontHandle* h = checkfonthandle(l, 1);
	font_free(*h);
	return 0;
}

static const luaL_Reg font_fun[] = {
	{"load", ml_font_load},
	{"size", ml_font_size},
	{"rect", ml_font_rect},
	{"free", ml_font_free},
	{NULL, NULL}
};	


// video

static bool video_initialized = false;

static int ml_video_init(lua_State* l) {
	if(video_initialized)
		return luaL_error(l, "video already initialized");
		
	int n = lua_gettop(l);
	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided to video.init");
	
	uint width = luaL_checkinteger(l, 1);
	uint height = luaL_checkinteger(l, 2);
	const char* name = luaL_checkstring(l, 3);

	video_init(width, height, name);
	video_initialized = true;

	return 0;
}	

static int ml_video_init_ex(lua_State* l) {
	if(video_initialized)
		return luaL_error(l, "video already initialized");

	int n = lua_gettop(l);
	if(n != 6)
		return luaL_error(l, "wrong number of arguments provided to video.init_ex");

	uint width = luaL_checknumber(l, 1);	
	uint height = luaL_checknumber(l, 2);
	uint v_width = luaL_checknumber(l, 3);
	uint v_height = luaL_checknumber(l, 4);
	const char* name = luaL_checkstring(l, 5);
	bool fullscreen = lua_toboolean(l, 6) != 0;

	video_init_ex(width, height, v_width, v_height, name, fullscreen);
	video_initialized = true;

	return 0;
}

static int ml_video_close(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video not initialized");

	int n = lua_gettop(l);
	if(n != 0)
		return luaL_error(l, "video.close() does not take any arguments");

	video_close();
	video_initialized = false;

	return 0;
}	

static int ml_video_present(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video not initialized");

	int n = lua_gettop(l);
	if(n != 0)
		return luaL_error(l, "video.present() does not take any arguments");

	bool res = system_update();
	video_present();	

	lua_pushboolean(l, res);
	return 1;
}

static bool _check_vec2(lua_State* l, int i, Vector2* v) {
	lua_getfield(l, i, "x");
	lua_getfield(l, i, "y");

	if(lua_isnumber(l, -1) && lua_isnumber(l, -2)) {
		v->x = lua_tonumber(l, -2);
		v->y = lua_tonumber(l, -1);
		lua_pop(l, 2);
		return true;
	}
	else {
		lua_pop(l, 2);
		return false;
	}
}

static bool _check_rect(lua_State* l, int i, RectF* r) {
	lua_getfield(l, i, "l");
	lua_getfield(l, i, "t");
	lua_getfield(l, i, "r");
	lua_getfield(l, i, "b");

	if(lua_isnumber(l, -1) && lua_isnumber(l, -2) 
		&& lua_isnumber(l, -3) && lua_isnumber(l, -4)) {
		r->left = lua_tonumber(l, -4);
		r->top = lua_tonumber(l, -3);
		r->right = lua_tonumber(l, -2);
		r->bottom = lua_tonumber(l, -1);
		lua_pop(l, 4);
		return true;
	}
	else {
		lua_pop(l, 4);
		return false;
	}
}

static bool _check_color(lua_State* l, int i, Color* c) {
	lua_getfield(l, i, "r");
	lua_getfield(l, i, "g");
	lua_getfield(l, i, "b");
	lua_getfield(l, i, "a");

	if(lua_isnumber(l, -1) && lua_isnumber(l, -2) 
		&& lua_isnumber(l, -3) && lua_isnumber(l, -4)) {
		uint r = lrintf(lua_tonumber(l, -4)*255.0) & 0xFF;
		uint g = lrintf(lua_tonumber(l, -3)*255.0) & 0xFF;
		uint b = lrintf(lua_tonumber(l, -2)*255.0) & 0xFF;
		uint a = lrintf(lua_tonumber(l, -1)*255.0) & 0xFF;
		*c = COLOR_RGBA(r, g, b, a);
		lua_pop(l, 4);
		return true;
	}
	else {
		lua_pop(l, 4);
		return false;
	}
}

static bool _get_dest(lua_State* l, int i, RectF* dest) {
	Vector2 vdest;
	RectF rdest;
	if(_check_vec2(l, i, &vdest)) {
		*dest = rectf(vdest.x, vdest.y, 0.0f, 0.0f);
		return true;
	}
	if(_check_rect(l, i, &rdest)) {
		*dest = rdest;
		return true;
	}	
	return false;
}

static int ml_video_draw_rect(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video not initialized");

	TexHandle* h = checktexhandle(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	if(layer > 15) 
		goto error;

	int n = lua_gettop(l);
	if(n == 3) {
		RectF dest;
		if(_get_dest(l, 3, &dest)) {
			video_draw_rect(*h, layer, NULL, &dest, COLOR_WHITE);
			return 0;
		}	
		goto error;
	}
	else if(n == 4) {
		RectF dest, src = rectf_null();
		if(lua_isnumber(l, 4)) {
			float rot = lua_tonumber(l, 4);
			if(_get_dest(l, 3, &dest)) {
				video_draw_rect_rotated(*h, layer, NULL, &dest,
					rot, COLOR_WHITE);
				return 0;
			}
			goto error;
		}
		Color c;
		if(_check_color(l, 4, &c)) {
			if(_get_dest(l, 3, &dest)) {
				video_draw_rect(*h, layer, NULL, &dest, c);
				return 0;
			}	
			goto error;
		}
		if(_check_rect(l, 3, &src)) {
			if(_get_dest(l, 4, &dest)) {
				video_draw_rect(*h, layer, &src, &dest,
					COLOR_WHITE);
				return 0;
			}
		}
		goto error;
	}
	else if(n == 5) {
		RectF dest, src = rectf_null();
		Color c;
		if(lua_isnumber(l, 4) && _check_color(l, 5, &c)) {
			float rot = lua_tonumber(l, 4);
			if(_get_dest(l, 3, &dest)) {
				video_draw_rect_rotated(*h, layer, NULL, 
					&dest, rot, COLOR_WHITE);
				return 0;
			}
			goto error;
		}
		if(!_check_rect(l, 3, &src) || !_get_dest(l, 4, &dest))
			goto error;
		if(lua_isnumber(l, 5)) {
			float rot = lua_tonumber(l, 5);
			video_draw_rect_rotated(*h, layer, &src, &dest,
				rot, COLOR_WHITE);
			return 0;	
		}
		if(_check_color(l, 5, &c)) {
			video_draw_rect(*h, layer, &src, &dest, c);
			return 0;
		}
		goto error;
	}
	else if(n == 6) {
		RectF dest, src;
		Color c;
		if(!_check_rect(l, 3, &src) || !_get_dest(l, 4, &dest)
			|| !_check_color(l, 6, &c) || !lua_isnumber(l, 5))
			goto error;
		float rot = lua_tonumber(l, 5);
		video_draw_rect_rotated(*h, layer, &src, &dest, rot, c);
		return 0;
	}
	else 
		return luaL_error(l, "wrong number of arguments provided to video.draw_rect");

	error:
		return luaL_error(l, "bad argument provided to video.draw_rect");
}

static int ml_video_draw_rect_centered(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video is not initialized");

	TexHandle* h = checktexhandle(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	RectF src = rectf_null();
	Vector2 dest;
	Color c;
	if(layer > 15) 
		goto error;

	int n = lua_gettop(l);

	if(n == 3) {
		if(_check_vec2(l, 3, &dest)) {
			gfx_draw_textured_rect(*h, layer, NULL, &dest, 0.0f, 1.0f,
				COLOR_WHITE);
			return 0;
		}
		goto error;
	}
	else if(n == 4) {
		if(lua_isnumber(l, 4)) {
			float rot = lua_tonumber(l, 4);
			if(_check_vec2(l, 3, &dest)) {
				gfx_draw_textured_rect(*h, layer, NULL, &dest, rot, 1.0f,
					COLOR_WHITE);
				return 0;	
			}
			goto error;
		}
		if(_check_color(l, 4, &c)) {
			if(_check_vec2(l, 3, &dest)) {
				gfx_draw_textured_rect(*h, layer, NULL, &dest, 0.0f, 1.0f, c);
				return 0;
			}
			goto error;
		}
		if(_check_rect(l, 3, &src)) {
			if(_check_vec2(l, 4, &dest)) {
				gfx_draw_textured_rect(*h, layer, &src, &dest, 0.0f, 1.0f,
					COLOR_WHITE);
				return 0;	
			}
		}
		goto error;
	}
	else if(n == 5) {
		if(lua_isnumber(l, 4)) {
			if(lua_isnumber(l, 5)) {
				float scale = lua_tonumber(l, 5);
				float rot = lua_tonumber(l, 4);
				if(_check_vec2(l, 3, &dest)) {
					gfx_draw_textured_rect(*h, layer, NULL, &dest,
						rot, scale, COLOR_WHITE);
					return 0;
				}	
				goto error;
			}	
			if(_check_color(l, 5, &c)) {
				float rot = lua_tonumber(l, 4);
				if(_check_vec2(l, 3, &dest)) {
					gfx_draw_textured_rect(*h, layer, NULL, &dest,
						rot, 1.0f, c);
					return 0;	
				}
			}	
			goto error;
		}
		if(lua_isnumber(l, 5)) {
			float rot = lua_tonumber(l, 5);
			if(!_check_rect(l, 3, &src) || !_check_vec2(l, 4, &dest))
				goto error;
			gfx_draw_textured_rect(*h, layer, &src, &dest, rot, 1.0f,
				COLOR_WHITE);
			return 0;
		}
		if(_check_color(l, 5, &c)) {
			if(!_check_rect(l, 3, &src) || !_check_vec2(l, 4, &dest))
				goto error;
			gfx_draw_textured_rect(*h, layer, &src, &dest, 0.0f, 1.0f, c);
			return 0;
		}
		goto error;
	}
	else if(n == 6) {
		if(lua_isnumber(l, 5) && lua_isnumber(l, 6)) {
			float rot = lua_tonumber(l, 5);
			float scale = lua_tonumber(l, 6);
			if(!_check_rect(l, 3, &src) || !_check_vec2(l, 4, &dest))
				goto error;
			gfx_draw_textured_rect(*h, layer, &src, &dest, rot, scale,
				COLOR_WHITE);
			return 0;
		}
		if(lua_isnumber(l, 4) && lua_isnumber(l, 5)) {
			float rot = lua_tonumber(l, 4);
			float scale = lua_tonumber(l, 5);
			if(!_check_color(l, 6, &c) || !_check_vec2(l, 3, &dest))
				goto error;
			gfx_draw_textured_rect(*h, layer, NULL, &dest, rot, scale, c);
			return 0;
		}
		if(!_check_rect(l, 3, &src) || !_check_vec2(l, 4, &dest)
			|| !lua_isnumber(l, 5) || !_check_color(l, 6, &c))
			goto error;
		float rot = lua_tonumber(l, 5);
		gfx_draw_textured_rect(*h, layer, &src, &dest, rot, 1.0f, c);
		return 0;
	}
	else if(n == 7) {
		if(!_check_rect(l, 3, &src) || !_check_vec2(l, 4, &dest)
			|| !lua_isnumber(l, 5) || !lua_isnumber(l, 6)
			|| !_check_color(l, 7, &c))
			goto error;
		float rot = lua_tonumber(l, 5);
		float scale = lua_tonumber(l, 6);
		gfx_draw_textured_rect(*h, layer, &src, &dest, rot, scale, c);
		return 0;
	}
	else
		return luaL_error(l, "wrong number of arguments provided to video.draw_rect_centered");
	
	error:
		return luaL_error(l, "bad argument provided to video.draw_rect_centered");
}

static int ml_video_draw_seg(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video is not initialized");

	int n = lua_gettop(l);
	Color c = COLOR_WHITE;
	if(n == 4) {
		if(!_check_color(l, 5, &c)) 
			goto error;
		n--;
	}

	if(n != 3)
		return luaL_error(l, "wrong number of arguments provided to video.draw_seg");

	uint layer = luaL_checkinteger(l, 1);
	if(layer > 15)
		goto error;

	Vector2 start, end;
	if(!_check_vec2(l, 2, &start) || !_check_vec2(l, 3, &end))
		goto error;

	video_draw_line(layer, &start, &end, c);
	return 0;

	error:
		return luaL_error(l, "bad argument provided to video.draw_seg");
}

static int ml_video_draw_text(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video is not initialized");

	int n = lua_gettop(l);
	Color c = COLOR_BLACK;
	if(n == 5) {
		if(!_check_color(l, 5, &c))
			goto error;
		n--;
	}

	if(n != 4)
		return luaL_error(l, "wrong number of arguments provided to video.draw_text");
	
	FontHandle* h = checkfonthandle(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	if(layer > 15)
		goto error;
	const char* text = luaL_checkstring(l, 3);
	Vector2 pos;
	if(!_check_vec2(l, 4, &pos))
		goto error;

	font_draw(*h, text, layer, &pos, c);
	return 0;

	error:
		return luaL_error(l, "bad argument provided to video.draw_text");
}

static int ml_video_draw_text_centered(lua_State* l) {
	if(!video_initialized)
		return luaL_error(l, "video is not initialized");

	int n = lua_gettop(l);
	if(n < 4 || n > 6)
		goto nargs_error;

	FontHandle* h = checkfonthandle(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	if(layer > 15)
		goto error;
	const char* text = luaL_checkstring(l, 3);
	Vector2 pos;
	if(!_check_vec2(l, 4, &pos))
		goto error;

	Color c;
	if(n == 4) {
		font_draw_ex(*h, text, layer, &pos, 1.0f, COLOR_BLACK);	
		return 0;
	}
	else if(n == 5) {
		if(lua_isnumber(l, 5)) {
			float scale = lua_tonumber(l, 5);
			font_draw_ex(*h, text, layer, &pos, scale, COLOR_BLACK);	
			return 0;
		}
		if(_check_color(l, 5, &c)) {
			font_draw_ex(*h, text, layer, &pos, 1.0f, c);	
			return 0;
		}
		goto error;
	}
	else if(n == 6) {
		if(lua_isnumber(l, 5) && _check_color(l, 6, &c)) {
			float scale = lua_tonumber(l, 5);
			font_draw_ex(*h, text, layer, &pos, scale, c);	
			return 0;
		}
	}
	error:
		return luaL_error(l, "bad argument provided to video.draw_text");

	nargs_error:
		return luaL_error(l, "wrong number of arguments provided to video.draw_text");
}

static const luaL_Reg video_fun[] = {
	{"init", ml_video_init},
	{"init_ex", ml_video_init_ex},
	{"close", ml_video_close},
	{"present", ml_video_present},
	{"draw_rect", ml_video_draw_rect},
	{"draw_rect_centered", ml_video_draw_rect_centered},
	{"draw_seg", ml_video_draw_seg},
	{"draw_text", ml_video_draw_text},
	{"draw_text_centered", ml_video_draw_text_centered},
	{NULL, NULL}
};

int malka_open_system(lua_State* l) {
	luaL_register(l, "time", time_fun);

	luaL_newmetatable(l, "_TexHandle.mt");
	luaL_register(l, "tex", tex_fun);

	luaL_newmetatable(l, "_FontHandle.mt");
	luaL_register(l, "font", font_fun);

	luaL_register(l, "video", video_fun);

	return 1;
}
