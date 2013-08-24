#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <memory.h>
#include <system.h>
#include <font.h>
#include <gfx_utils.h>
#include <gui.h>
#include <particles.h>
#include <tilemap.h>

#ifdef __APPLE__
#include <vfont.h>
#endif

extern void _new_vec2(lua_State* l, double x, double y);
extern void _new_rect(lua_State* l, double _l, double t,
	double r, double b);
extern void _new_rgba(lua_State* l, double r, double g,
	double b, double a);

// time

static int ml_time_ms_current(lua_State* l) {
	checkargs(0, "time.ms_current");
	lua_pushnumber(l, time_ms_current());
	return 1;
}

static int ml_time_ms(lua_State* l) {
	checkargs(0, "time.ms");
	lua_pushnumber(l, time_ms());
	return 1;
}

static int ml_time_s(lua_State* l) {
	checkargs(0, "time.s");
	lua_pushnumber(l, (double)time_ms() / 1000.0);
	return 1;
}

static int ml_time_dt(lua_State* l) {
	checkargs(0, "time.dt");
	lua_pushnumber(l, time_delta());
	return 1;
}

static int ml_time_fps(lua_State* l) {
	checkargs(0, "time.fps");
	lua_pushinteger(l, time_fps());
	return 1;
}

static const luaL_Reg time_fun[] = {
	{"ms_current", ml_time_ms_current},
	{"ms", ml_time_ms},
	{"s", ml_time_s},
	{"dt", ml_time_dt},
	{"fps", ml_time_fps},
	{NULL, NULL}
};	


// texture

#ifdef _DEBUG
#define checktexhandle(l, i) \
	(TexHandle*)luaL_checkudata(l, i, "_TexHandle.mt")
#else
#define checktexhandle(l, i) \
	(TexHandle*)lua_touserdata(l, i)
#endif

void _new_texhandle(lua_State* l, TexHandle h) {
	TexHandle* t = (TexHandle*)lua_newuserdata(l, sizeof(TexHandle));
	*t = h;
	luaL_getmetatable(l, "_TexHandle.mt");
	lua_setmetatable(l, -2);
}

static int ml_tex_load(lua_State* l) {
	checkargs(1, "tex.load");
	const char* filename = luaL_checkstring(l, 1);
	TexHandle h = tex_load(filename);
	_new_texhandle(l, h);
	return 1;
}

static int ml_tex_size(lua_State* l) {
	checkargs(1, "tex.size");
	TexHandle* t = checktexhandle(l, 1);
	uint w, h;
	tex_size(*t, &w, &h);
	_new_vec2(l, (double)w, (double)h);
	return 1;
}

static int ml_tex_free(lua_State* l) {
	checkargs(1, "tex.free");
	TexHandle* h = checktexhandle(l, 1);
	tex_free(*h);
	return 0;
}

static int ml_tex_scale(lua_State* l) {
	checkargs(2, "tex.scale");
	TexHandle* h = checktexhandle(l, 1);
	float s = luaL_checknumber(l, 2);
	tex_scale(*h, s);
	return 0;
}

static const luaL_Reg tex_fun[] = {
	{"load", ml_tex_load},
	{"size", ml_tex_size},
	{"free", ml_tex_free},
	{"scale", ml_tex_scale},
	{NULL, NULL}
};	


// font

#define checkfonthandle(l, i) \
	(FontHandle*)luaL_checkudata(l, i, "_FontHandle.mt")

static void _new_fonthandle(lua_State* l, FontHandle h) {
	FontHandle* t = (FontHandle*)lua_newuserdata(l, sizeof(FontHandle));
	*t = h;
	luaL_getmetatable(l, "_FontHandle.mt");
	lua_setmetatable(l, -2);
}

static int ml_font_load(lua_State* l) {
	int n = lua_gettop(l);
	double scale = 1.0;
	const char* prefix = NULL;

    const char* filename = luaL_checkstring(l, 1);
    
	if(n == 2) {
		if(lua_isnumber(l, 2)) {
			scale = lua_tonumber(l, 2);
		}
		else {
			prefix = luaL_checkstring(l, 2);		
		}
	}
	else if(n == 3) {
		scale = luaL_checknumber(l, 2);
		prefix = luaL_checkstring(l, 3);
	}
	else if(n != 1)
		goto error;

	FontHandle h = font_load_exp(filename, (float)scale, prefix);
	_new_fonthandle(l, h);
	return 1;

error:
	return luaL_error(l, "wrong number of arguments provided to font.load");
}

static int ml_font_size(lua_State* l) {
	checkargs(2, "font.size");
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
	checkargs(1, "font.free");
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

#define checkvideoinit() \
	if(!video_initialized) \
		return luaL_error(l, "video not initialized")

#define checkvideodeinit() \
	if(video_initialized) \
		return luaL_error(l, "video already initialized")

static bool video_initialized = false;

static int ml_video_native_resolution(lua_State* l) {
	checkargs(0, "video.native_resolution");

	uint width, height;
	video_get_native_resolution(&width, &height);

	lua_pushinteger(l, width);
	lua_pushinteger(l, height);
	return 2;
}

static int ml_video_init(lua_State* l) {
	checkvideodeinit();
	checkargs(3, "video.init");
	uint width = luaL_checkinteger(l, 1);
	uint height = luaL_checkinteger(l, 2);
	const char* name = luaL_checkstring(l, 3);

	video_init(width, height, name);
	video_initialized = true;

	return 0;
}	

static int ml_video_init_ex(lua_State* l) {
	checkvideodeinit();
	checkargs(6, "video.init_ex");
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

static int ml_video_init_exr(lua_State* l) {
	checkvideodeinit();
	checkargs(6, "video.init_exr");
	uint width = luaL_checknumber(l, 1);	
	uint height = luaL_checknumber(l, 2);
	uint v_width = luaL_checknumber(l, 3);
	uint v_height = luaL_checknumber(l, 4);
	const char* name = luaL_checkstring(l, 5);
	bool fullscreen = lua_toboolean(l, 6) != 0;

	video_init_exr(width, height, v_width, v_height, name, fullscreen);
	video_initialized = true;

	return 0;
}

static int ml_video_close(lua_State* l) {
	checkvideoinit();
	checkargs(0, "video.close");
	video_close();
	video_initialized = false;

	return 0;
}	

bool _check_color(lua_State* l, int i, Color* c) {
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

static int ml_video_clear_color(lua_State* l) {
	checkargs(1, "video.clear_color");

	Color c = COLOR_RGBA(255, 255, 255, 255);
	_check_color(l, 1, &c);
	video_clear_color(c);

	return 0;
}

static int ml_video_present(lua_State* l) {
	checkargs(0, "video.present");
	bool res = system_update();
	video_present();	

	lua_pushboolean(l, res);
	return 1;
}

bool _check_vec2(lua_State* l, int i, Vector2* v) {
	if(lua_objlen(l, i) == 2) {
		lua_rawgeti(l, i, 1);
		lua_rawgeti(l, i, 2);

		v->x = lua_tonumber(l, -2);
		v->y = lua_tonumber(l, -1);
		lua_pop(l, 2);
		return true;
	}
	else {
		return false;
	}
}

bool _check_rect(lua_State* l, int i, RectF* r) {
	if(lua_objlen(l, i) == 4) {
		lua_rawgeti(l, i, 1);
		lua_rawgeti(l, i, 2);
		lua_rawgeti(l, i, 3);
		lua_rawgeti(l, i, 4);

		r->left = lua_tonumber(l, -4);
		r->top = lua_tonumber(l, -3);
		r->right = lua_tonumber(l, -2);
		r->bottom = lua_tonumber(l, -1);
		lua_pop(l, 4);
		return true;
	}
	else {
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
		if(_check_rect(l, 3, &src)) {
			if(_get_dest(l, 4, &dest)) {
				video_draw_rect(*h, layer, &src, &dest,
					COLOR_WHITE);
				return 0;
			}
		}
		Color c;
		if(_check_color(l, 4, &c)) {
			if(_get_dest(l, 3, &dest)) {
				video_draw_rect(*h, layer, NULL, &dest, c);
				return 0;
			}	
			goto error;
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
	int n = lua_gettop(l);
	Color c = COLOR_WHITE;
	if(n == 4) {
		if(!_check_color(l, 4, &c)) 
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
		return luaL_error(l, "bad argument provided to video.draw_text_centered");

	nargs_error:
		return luaL_error(l, "wrong number of arguments provided to video.draw_text_centered");
}

static int ml_video_draw_text_rotated(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 5 || n > 7)
		goto nargs_error;

	FontHandle* h = checkfonthandle(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	if(layer > 15)
		goto error;
	const char* text = luaL_checkstring(l, 3);
	Vector2 pos;
	if(!_check_vec2(l, 4, &pos))
		goto error;

	float rot = luaL_checknumber(l, 5);

	Color c;
	if(n == 5) {
		font_draw_rot(*h, text, layer, &pos, 1.0f, rot, COLOR_BLACK);
		return 0;
	}
	else if(n == 6) {
		if(lua_isnumber(l, 6)) {
			float scale = lua_tonumber(l, 6);
			font_draw_rot(*h, text, layer, &pos, scale, rot, COLOR_BLACK);
			return 0;
		}
		else if(_check_color(l, 6, &c)) {
			font_draw_rot(*h, text, layer, &pos, 1.0f, rot, c);
			return 0;
		}
		goto error;
	}
	else if(n == 7) {
		if(lua_isnumber(l, 6) && _check_color(l, 7, &c)) {
			float scale = lua_tonumber(l, 6);
			font_draw_rot(*h, text, layer, &pos, scale, rot, c);
			return 0;
		}
	}

	error:
		return luaL_error(l, "bad argument provided to video.draw_text_rotated");

	nargs_error:
		return luaL_error(l, "wrong number of arguments provided to video.draw_text_rotated");
}

static int ml_video_set_blendmode(lua_State* l) {
	checkargs(2, "video.set_blendmode");

	uint layer = luaL_checkinteger(l, 1);
	const char* mode = luaL_checkstring(l, 2);

	BlendMode bmode = BM_NORMAL;
	if(strcmp(mode, "add") == 0)
		bmode = BM_ADD;
	else if(strcmp(mode, "multiply") == 0)
		bmode = BM_MULTIPLY;
	else if(strcmp(mode, "normal") == 0)
		bmode = BM_NORMAL;
	else
		return luaL_error(l, "unrecognised blending mode");

	video_set_blendmode(layer, bmode);

	return 0;
}

static float ml_transforms[16][6];

static int ml_video_set_transform(lua_State* l) {
	checkargs(2, "video.set_transform");

	uint layer = luaL_checkinteger(l, 1); 
	assert(layer < 16);

	if(lua_istable(l, 2)) {
		for(uint i = 0; i < 6; ++i) {
			lua_rawgeti(l, 2, i+1);
			ml_transforms[layer][i] = luaL_checknumber(l, 3);
			lua_pop(l, 1);
		}
		video_set_transform(layer, ml_transforms[layer]);
	}
	else {
		video_set_transform(layer, NULL);
	}

	return 0;
}

static const luaL_Reg video_fun[] = {
	{"native_resolution", ml_video_native_resolution},
	{"init", ml_video_init},
	{"init_ex", ml_video_init_ex},
	{"init_exr", ml_video_init_exr},
	{"close", ml_video_close},
	{"clear_color", ml_video_clear_color},
	{"present", ml_video_present},
	{"draw_rect", ml_video_draw_rect},
	{"draw_rect_centered", ml_video_draw_rect_centered},
	{"draw_seg", ml_video_draw_seg},
	{"draw_text", ml_video_draw_text},
	{"draw_text_centered", ml_video_draw_text_centered},
	{"draw_text_rotated", ml_video_draw_text_rotated},
	{"set_blendmode", ml_video_set_blendmode},
	{"set_transform", ml_video_set_transform},
	{NULL, NULL}
};

#ifdef __APPLE__

// vfont

static int ml_vfont_init(lua_State* l) {
	int n = lua_gettop(l);
	if(n == 0) {
		vfont_init();
		return 0;
	}
	else if(n == 2) {
		uint w = luaL_checkinteger(l, 1);
		uint h = luaL_checkinteger(l, 2);
		vfont_init_ex(w, h);
		return 0;
	}

	return luaL_error(l, "bad args to vfont.init");
}

static int ml_vfont_close(lua_State* l) {
	checkargs(0, "vfont.close");
	vfont_close();
	return 0;
}

static int ml_vfont_select(lua_State* l) {
	checkargs(2, "vfont.select");
	const char* name = luaL_checkstring(l, 1);
	float size = luaL_checknumber(l, 2);
	vfont_select(name, size);
	return 0;
}

static int ml_vfont_draw(lua_State* l) {
	int n = lua_gettop(l);
	const char* string = luaL_checkstring(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	Vector2 pos;
	if(!_check_vec2(l, 3, &pos))
		goto error;
	Color c = COLOR_BLACK;

	if(n == 4) 
		if(!_check_color(l, 4, &c))
			goto error;

	if(n == 3 || n == 4) {
		vfont_draw(string, layer, pos, c);
		return 0;
	}	

error:
	return luaL_error(l, "bad args to vfont.draw");
}

static int ml_vfont_draw_input(lua_State* l) {
	int n = lua_gettop(l);
	const char* string = luaL_checkstring(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	Vector2 pos;
	if(!_check_vec2(l, 3, &pos))
		goto error;
	Color c = COLOR_BLACK;

	if(n == 4) 
		if(!_check_color(l, 4, &c))
			goto error;

	if(n == 3 || n == 4) {
		vfont_draw_input(string, layer, pos, c);
		return 0;
	}	

error:
	return luaL_error(l, "bad args to vfont.draw_input");
}

static int ml_vfont_size(lua_State* l) {
	checkargs(1, "vfont.size");
	const char* string = luaL_checkstring(l, 1);

	Vector2 size = vfont_size(string);
	_new_vec2(l, size.x, size.y);
	return 1;
}

static int ml_vfont_precache(lua_State* l) {
	checkargs(1, "vfont.precache");
	const char* string = luaL_checkstring(l, 1);
	vfont_precache(string);
	return 0;
}

static int ml_vfont_invalidate(lua_State* l) {
	checkargs(1, "vfont.invalidate");
	const char* string = luaL_checkstring(l, 1);
	vfont_cache_invalidate(string);
	return 0;
}

static int ml_vfont_invalidate_all(lua_State* l) {
    checkargs(0, "vfont.invalidate_all");
    vfont_invalidate_all();
    return 0;
}

static int ml_vfont_invalidate_nonstrict(lua_State* l) {
	checkargs(1, "vfont.invalidate_nonstrict");
	const char* string = luaL_checkstring(l, 1);
	vfont_cache_invalidate_ex(string, false);
	return 0;
}

static const luaL_Reg vfont_fun[] = {
	{"init", ml_vfont_init},
	{"close", ml_vfont_close},
	{"select", ml_vfont_select},
	{"draw", ml_vfont_draw},
	{"draw_input", ml_vfont_draw_input},
	{"size", ml_vfont_size},
	{"precache", ml_vfont_precache},
	{"invalidate", ml_vfont_invalidate},
	{"invalidate_nonstrict", ml_vfont_invalidate_nonstrict},
    {"invalidate_all", ml_vfont_invalidate_all},
	{NULL, NULL}
};

#endif

// sound

#define checksoundhandle(l, i) \
	(SoundHandle*)luaL_checkudata(l, i, "_SoundHandle.mt")

static void _new_soundhandle(lua_State* l, SoundHandle h) {
	SoundHandle* t = (SoundHandle*)lua_newuserdata(l, sizeof(SoundHandle));
	*t = h;
	luaL_getmetatable(l, "_SoundHandle.mt");
	lua_setmetatable(l, -2);
}

#define checksourcehandle(l, i) \
	(SourceHandle*)luaL_checkudata(l, i, "_SourceHandle.mt")

static void _new_sourcehandle(lua_State* l, SourceHandle h) {
	SourceHandle* t = (SourceHandle*)lua_newuserdata(l, sizeof(SourceHandle));
	*t = h;
	luaL_getmetatable(l, "_SourceHandle.mt");
	lua_setmetatable(l, -2);
}

static bool sound_initialized = false;

#define checksoundinit() \
	if(!sound_initialized) \
		return luaL_error(l, "sound not initialized")

#define checksounddeinit() \
	if(sound_initialized) \
		return luaL_error(l, "sound already initialized")

static int ml_sound_init(lua_State* l) {
	checksounddeinit();
	checkargs(0, "sound.init");
	sound_init();
	sound_initialized = true;
	return 0;
}

static int ml_sound_close(lua_State* l) {
	checksoundinit();
	checkargs(0, "sound.close");
	sound_close();
	sound_initialized = false;
	return 0;
}

static int ml_sound_update(lua_State* l) {
	checkargs(0, "sound.update");
	sound_update();
	return 0;
}

static int ml_sound_load_stream(lua_State* l) {
	checkargs(1, "sound.load_stream");
	const char* filename = luaL_checkstring(l, 1);
	SoundHandle h = sound_load_stream(filename);
	_new_soundhandle(l, h);
	return 1;
}

static int ml_sound_load_sample(lua_State* l) {
	checkargs(1, "sound.load_sample");
	const char* filename = luaL_checkstring(l, 1);
	SoundHandle h = sound_load_sample(filename);
	_new_soundhandle(l, h);

	return 1;
}

static int ml_sound_free(lua_State* l) {
	checkargs(1, "sound.free");
	SoundHandle* h = checksoundhandle(l, 1);
	sound_free(*h);
	return 0;
}

static int ml_sound_set_volume(lua_State* l) {
	checkargs(2, "sound.set_volume");
	SoundHandle* h = checksoundhandle(l, 1);
	double vol = luaL_checknumber(l, 2);
	
	sound_set_volume(*h, (float)vol);

	return 0;
}

static int ml_sound_volume(lua_State* l) {
	checkargs(1, "sound.volume");
	SoundHandle* h = checksoundhandle(l, 1);
	lua_pushnumber(l, sound_get_volume(*h));
	return 1;
}

static int ml_sound_length(lua_State* l) {
	checkargs(1, "sound.length");
	SoundHandle* h = checksoundhandle(l, 1);
	lua_pushnumber(l, sound_get_length(*h));
	return 1;
}

static int ml_sound_play(lua_State* l) {
	int n = lua_gettop(l);
	bool loop = false;
	if(n == 2) {
		loop = lua_toboolean(l, 2);
		n--;
	}

	if(n != 1)
		return luaL_error(l, "wrong number of arguments provided to sound.play");

	SoundHandle* h = checksoundhandle(l, 1);
	SourceHandle s = sound_play_ex(*h, loop);

	if(s)
		_new_sourcehandle(l, s);
	else
		lua_pushnil(l);

	return 1;
}

static int ml_sound_pause(lua_State* l) {
	checkargs(1, "sound.pause");
	SourceHandle* s = checksourcehandle(l, 1);
	sound_pause_ex(*s);
	return 0;
}

static int ml_sound_resume(lua_State* l) {
	checkargs(1, "sound.resume");
	SourceHandle* s = checksourcehandle(l, 1);
	sound_resume_ex(*s);
	return 0;
}

static int ml_sound_stop(lua_State* l) {
	checkargs(1, "sound.stop");
	SourceHandle* s = checksourcehandle(l, 1);
	sound_stop_ex(*s);
	return 0;
}

static int ml_sound_set_src_volume(lua_State* l) {
	checkargs(2, "sound.set_src_volume");
	SourceHandle* h = checksourcehandle(l, 1);
	double vol = luaL_checknumber(l, 2);
	sound_set_volume_ex(*h, (float)vol);
	return 0;
}

static int ml_sound_src_volume(lua_State* l) {
	checkargs(1, "sound.src_volume");
	SourceHandle* h = checksourcehandle(l, 1);
	lua_pushnumber(l, sound_get_volume_ex(*h));
	return 1;
}

static int ml_sound_set_pos(lua_State* l) {
	checkargs(2, "sound.set_pos");
	SourceHandle* h = checksourcehandle(l, 1);
	double pos = luaL_checknumber(l, 2);
	sound_set_pos_ex(*h, (float)pos);
	return 0;
}

static int ml_sound_pos(lua_State* l) {
	checkargs(1, "sound.pos");
	SourceHandle* h = checksourcehandle(l, 1);
	lua_pushnumber(l, sound_get_pos_ex(*h));
	return 1;
}

static const luaL_Reg sound_fun[] = {
	{"init", ml_sound_init},
	{"close", ml_sound_close},
	{"update", ml_sound_update},
	{"load_stream", ml_sound_load_stream},
	{"load_sample", ml_sound_load_sample},
	{"free", ml_sound_free},
	{"set_volume", ml_sound_set_volume},
	{"volume", ml_sound_volume},
	{"length", ml_sound_length},
	{"play", ml_sound_play},
	{"pause", ml_sound_pause},
	{"resume", ml_sound_resume},
	{"stop", ml_sound_stop},
	{"set_src_volume", ml_sound_set_src_volume},
	{"src_volume", ml_sound_src_volume},
	{"set_pos", ml_sound_set_pos},
	{"pos", ml_sound_pos},
	{NULL, NULL}
};	


// input

static int ml_key_pressed(lua_State* l) {
	checkargs(1, "key.pressed");
	uint key = luaL_checkinteger(l, 1);
	if(key >= KEY_COUNT)
		return luaL_error(l, "bad key");
	lua_pushboolean(l, key_pressed((Key)key));
	return 1;
}

static int ml_key_down(lua_State* l) {
	checkargs(1, "key.down");
	uint key = luaL_checkinteger(l, 1);
	if(key >= KEY_COUNT)
		return luaL_error(l, "bad key");
	lua_pushboolean(l, key_down((Key)key));
	return 1;
}

static int ml_key_up(lua_State* l) {
	checkargs(1, "key.up");
	uint key = luaL_checkinteger(l, 1);
	if(key >= KEY_COUNT)
		return luaL_error(l, "bad key");
	lua_pushboolean(l, key_up((Key)key));
	return 1;
}

static int ml_char_pressed(lua_State* l) {
	checkargs(1, "char.pressed");
	char c;
	if(lua_isstring(l, 1)) {
		const char* str = lua_tostring(l, 1);
		if(strlen(str) > 1)
			return luaL_error(l, "bad char");
		c = str[0];	
	}
	else
		c = (char)lua_tointeger(l, 1);
	lua_pushboolean(l, char_pressed(c));
	return 1;
}

static int ml_char_down(lua_State* l) {
	checkargs(1, "char.down");
	char c;
	if(lua_isstring(l, 1)) {
		const char* str = lua_tostring(l, 1);
		if(strlen(str) > 1)
			return luaL_error(l, "bad char");
		c = str[0];	
	}
	else
		c = (char)lua_tointeger(l, 1);
	lua_pushboolean(l, char_down(c));
	return 1;
}

static int ml_char_up(lua_State* l) {
	checkargs(1, "char.up");
	char c;
	if(lua_isstring(l, 1)) {
		const char* str = lua_tostring(l, 1);
		if(strlen(str) > 1)
			return luaL_error(l, "bad char");
		c = str[0];	
	}
	else
		c = (char)lua_tointeger(l, 1);
	lua_pushboolean(l, char_up(c));
	return 1;
}

static int ml_mouse_pressed(lua_State* l) {
	checkargs(1, "mouse.pressed");
	uint mkey = luaL_checkinteger(l, 1);
	if(mkey >= MBTN_COUNT)
		return luaL_error(l, "bad mouse button");
	lua_pushboolean(l, mouse_pressed((MouseButton)mkey));
	return 1;
}

static int ml_mouse_down(lua_State* l) {
	checkargs(1, "mouse.down");
	uint mkey = luaL_checkinteger(l, 1);
	if(mkey >= MBTN_COUNT)
		return luaL_error(l, "bad mouse button");
	lua_pushboolean(l, mouse_down((MouseButton)mkey));
	return 1;
}

static int ml_mouse_up(lua_State* l) {
	checkargs(1, "mouse.up");
	uint mkey = luaL_checkinteger(l, 1);
	if(mkey >= MBTN_COUNT)
		return luaL_error(l, "bad mouse button");
	lua_pushboolean(l, mouse_up((MouseButton)mkey));
	return 1;
}

static int ml_mouse_pos(lua_State* l) {
	checkargs(0, "mouse.pos");
	uint x, y;
	mouse_pos(&x, &y);
	_new_vec2(l, (double)x, (double)y);
	return 1;
}

static int ml_touch_count(lua_State* l) {
	checkargs(0, "touch.count");
	lua_pushinteger(l, touches_count());
	return 1;
}

static int ml_touch_get(lua_State* l) {
	checkargs(1, "touch.get");
	
	uint i = luaL_checkinteger(l, 1);
	Touch* t = touches_get();
	if(!t || i >= touches_count()) {
		lua_pushnil(l);
		return 1;
	}

	lua_createtable(l, 0, 3);
	int table = lua_gettop(l);

	_new_vec2(l, t[i].pos.x, t[i].pos.y);
	lua_setfield(l, table, "pos");
	_new_vec2(l, t[i].hit_pos.x, t[i].hit_pos.y);
	lua_setfield(l, table, "hit_pos");
	lua_pushnumber(l, t[i].hit_time);
	lua_setfield(l, table, "hit_time");

	return 1;
}

static const luaL_Reg key_fun[] = {
	{"pressed", ml_key_pressed},
	{"down", ml_key_down},
	{"up", ml_key_up},
	{NULL, NULL}
};

static const luaL_Reg char_fun[] = {
	{"pressed", ml_char_pressed},
	{"down", ml_char_down},
	{"up", ml_char_up},
	{NULL, NULL}
};	

static const luaL_Reg mouse_fun[] = {
	{"pressed", ml_mouse_pressed},
	{"down", ml_mouse_down},
	{"up", ml_mouse_up},
	{"pos", ml_mouse_pos},
	{NULL, NULL}
};	

static const luaL_Reg touch_fun[] = {
	{"count", ml_touch_count},
	{"get", ml_touch_get},
	{NULL, NULL}
};

static const char* key_names[] = {
	"_up", "_down", "_left", "_right", "a", "b", "pause", "quit"
};	

static const char* mbtn_names[] = {
	"primary", "secondary", "middle", "wheelup", "wheeldown"
};	


// text input

#ifdef TARGET_IOS

static int ml_txtinput_start(lua_State* l) {
	checkargs(0, "txtinput.start");
	txtinput_start();
	return 0;
}

static int ml_txtinput_get(lua_State* l) {
	checkargs(0, "txtinput.get");
	lua_pushstring(l, txtinput_get());
	return 1;
}

static int ml_txtinput_set(lua_State* l) {
	checkargs(1, "txtinput.set");
	const char* text = luaL_checkstring(l, 1);
	txtinput_set(text);
	return 0;
}

static int ml_txtinput_did_end(lua_State* l) {
	checkargs(0, "txtinput.did_end");
	const char* str = txtinput_did_end();
	if(str)
		lua_pushstring(l, str);
	else
		lua_pushnil(l);
	return 1;
}

static int ml_txtinput_finish(lua_State* l) {
	checkargs(0, "txtinput.finish");
	lua_pushstring(l, txtinput_end());
	return 1;
}

static int ml_txtinput_clear(lua_State* l) {
	checkargs(0, "txtinput.clear");
	txtinput_clear();
	return 0;
}

static const luaL_Reg txtinput_fun[] = {
	{"start", ml_txtinput_start},
	{"get", ml_txtinput_get},
	{"set", ml_txtinput_set},
	{"did_end", ml_txtinput_did_end},
	{"finish", ml_txtinput_finish},
	{"clear", ml_txtinput_clear},
	{NULL, NULL}
};

#else

static const luaL_Reg txtinput_fun[] = {
	{NULL, NULL}
};

#endif


// orientation

static int ml_orientation_current(lua_State* l) {
	checkargs(0, "orientation.current");
	lua_pushinteger(l, orientation_current());
	return 1;
}

static int ml_orientation_did_change(lua_State* l) {
	checkargs(0, "orientation.did_change");

	DevOrient new;
	float anim_start;
	float anim_len;
	bool res = orientation_change(&new, &anim_start, &anim_len);

	if(res) {
		lua_pushinteger(l, new);
		lua_pushnumber(l, anim_start);
		lua_pushnumber(l, anim_len);
		return 3;
	}
	else {
		lua_pushnil(l);
		return 1;
	}
}

static int ml_orientation_angle(lua_State* l) {
	checkargs(1, "orientation.angle");
	DevOrient orient = luaL_checkinteger(l, 1);
	switch(orient) {
		case ORIENT_LANDSCAPE_RIGHT:
			lua_pushnumber(l, 0.0);
			break;
		case ORIENT_PORTRAIT_UPSIDE_DOWN:
			lua_pushnumber(l, PI / 2.0);
			break;
		case ORIENT_LANDSCAPE_LEFT:
			lua_pushnumber(l, PI);
			break;
		case ORIENT_PORTRAIT:
			lua_pushnumber(l, PI / 2.0 * 3.0);
			break;
		default:
			return luaL_error(l, "unknown orientation");
	}
	return 1;
}

extern void _new_vec2(lua_State* l, double x, double y);

static int ml_orientation_basis(lua_State* l) {
	checkargs(1, "orientation.angle");
	DevOrient orient = luaL_checkinteger(l, 1);
	switch(orient) {
		case ORIENT_LANDSCAPE_RIGHT:
			_new_vec2(l, 1.0, 0.0);
			_new_vec2(l, 0.0, 1.0);
			break;
		case ORIENT_PORTRAIT_UPSIDE_DOWN:
			_new_vec2(l, 0.0, -1.0);
			_new_vec2(l, 1.0, 0.0);
			break;
		case ORIENT_LANDSCAPE_LEFT:
			_new_vec2(l, -1.0, 0.0);
			_new_vec2(l, 0.0, -1.0);
			break;
		case ORIENT_PORTRAIT:
			_new_vec2(l, 0.0, 1.0);
			_new_vec2(l, -1.0, 0.0);
			break;
		default:
			return luaL_error(l, "unknown orientation");
	}
	return 2;
}

static const luaL_Reg orientation_fun[] = {
	{"current", ml_orientation_current},
	{"did_change", ml_orientation_did_change},
	{"angle", ml_orientation_angle},
	{"basis", ml_orientation_basis},
	{NULL, NULL}
};

// acceleration

static lua_State* cb_l;
static int cb_shake_ref = -1;

static void _shake_cb(void) {
	lua_getref(cb_l, cb_shake_ref);
	assert(lua_isfunction(cb_l, -1));
	lua_call(cb_l, 0, 0);
}

static int ml_acceleration_shake_callback(lua_State* l) {
	checkargs(1, "acceleration.shake_callback");

	if(cb_shake_ref != -1) {
		lua_unref(l, cb_shake_ref);
		cb_shake_ref = -1;
	}

	if(lua_isnil(l, 1)) {
		acc_shake_cb(NULL);
	}
	else if(lua_isfunction(l, 1)) {
		cb_l = l;
		cb_shake_ref = lua_ref(l, 1);
		acc_shake_cb(_shake_cb);
	}
	else {
		return luaL_error(l, "wrong callback type");
	}

	return 0;
}

static const luaL_Reg acceleration_fun[] = {
	{"shake_callback", ml_acceleration_shake_callback},
	{NULL, NULL}
};

// runstate

static int cb_background_ref = -1;
static int cb_foreground_ref = -1;

static void _background_cb(void) {
	lua_getref(cb_l, cb_background_ref);
	assert(lua_isfunction(cb_l, -1));
	lua_call(cb_l, 0, 0);
}

static void _foreground_cb(void) {
	lua_getref(cb_l, cb_foreground_ref);
	assert(lua_isfunction(cb_l, -1));
	lua_call(cb_l, 0, 0);
}

static int ml_runstate_background_callback(lua_State* l) {
	checkargs(1, "runstate.background_callback");

	if(cb_background_ref != -1) {
		lua_unref(l, cb_background_ref);
		cb_background_ref = -1;
	}

	if(lua_isnil(l, 1)) {
		runstate_background_cb(NULL);
	}
	else if(lua_isfunction(l, 1)) {
		cb_l = l;
		cb_background_ref = lua_ref(l, 1);
		runstate_background_cb(_background_cb);	
	}
	else {
		return luaL_error(l, "wrong callback type");
	}

	return 0;
}

static int ml_runstate_foreground_callback(lua_State* l) {
	checkargs(1, "runstate.foreground_callback");

	if(cb_foreground_ref != -1) {
		lua_unref(l, cb_foreground_ref);
		cb_foreground_ref = -1;
	}

	if(lua_isnil(l, 1)) {
		runstate_foreground_cb(NULL);
	}
	else if(lua_isfunction(l, 1)) {
		cb_l = l;
		cb_foreground_ref = lua_ref(l, 1);
		runstate_foreground_cb(_foreground_cb);	
	}
	else {
		return luaL_error(l, "wrong callback type");
	}

	return 0;
}

static const luaL_Reg runstate_fun[] = {
	{"background_callback", ml_runstate_background_callback},
	{"foreground_callback", ml_runstate_foreground_callback},
	{NULL, NULL}
};

// gui

static void _push_rect(lua_State* l, RectF rect) {
	double _l = rect.left;
	double t = rect.top;
	double r = rect.right;
	double b = rect.bottom;
	_new_rect(l, _l, t, r, b);
}

static void _push_color(lua_State* l, Color c) {
	byte r, g, b, a;
	COLOR_DECONSTRUCT(c, r, g, b, a);
	double dr = (double)r / 255.0;
	double dg = (double)g / 255.0;
	double db = (double)b / 255.0;
	double da = (double)a / 255.0;
	_new_rgba(l, dr, dg, db, da);
}

#define _setrectfield(l, i, name, src) \
	_push_rect(l, src); \
	lua_setfield(l, i, name)

static void _make_guidesc(lua_State* l, const GuiDesc* desc) {
	lua_createtable(l, 0, 15);
	int table = lua_gettop(l);
	_new_texhandle(l, desc->texture);
	lua_setfield(l, table, "texture");
	_new_fonthandle(l, desc->font);
	lua_setfield(l, table, "font");
	_push_color(l, desc->text_color);
	lua_setfield(l, table, "text_color");
	lua_pushinteger(l, desc->first_layer);
	lua_setfield(l, table, "first_layer");
	lua_pushinteger(l, desc->second_layer);
	lua_setfield(l, table, "second_layer");
	lua_pushinteger(l, desc->text_layer);
	lua_setfield(l, table, "text_layer");
	_setrectfield(l, table, "src_button_down", desc->src_button_down);
	_setrectfield(l, table, "src_button_up", desc->src_button_up);
	_setrectfield(l, table, "src_switch_on_up", desc->src_switch_on_up);
	_setrectfield(l, table, "src_switch_on_down", desc->src_switch_on_down);
	_setrectfield(l, table, "src_switch_off_up", desc->src_switch_off_up);
	_setrectfield(l, table, "src_switch_off_down", desc->src_switch_off_down);
	_setrectfield(l, table, "src_slider", desc->src_slider);
	_setrectfield(l, table, "src_slider_knob_up", desc->src_slider_knob_up);
	_setrectfield(l, table, "src_slider_knob_down", desc->src_slider_knob_down);
}

#define _getrectfield(l, i, name, dest) \
	lua_getfield(l, i, name); \
	t = lua_gettop(l); \
	if(!_check_rect(l, t, &dest)) return false

static bool _read_guidesc(lua_State* l, GuiDesc* desc) {
	if(!lua_istable(l, -1))
		return false;
	int t, table = lua_gettop(l);
	lua_getfield(l, table, "texture");
	TexHandle* h = checktexhandle(l, -1);
	desc->texture =	*h;
	lua_getfield(l, table, "font");
	FontHandle* f = checkfonthandle(l, -1);
	desc->font = *f;
	lua_getfield(l, table, "text_color");
	t = lua_gettop(l);
	if(!_check_color(l, t, &desc->text_color))
		return false;
	lua_getfield(l, table, "first_layer");
	desc->first_layer = luaL_checkinteger(l, -1);
	lua_getfield(l, table, "second_layer");
	desc->second_layer = luaL_checkinteger(l, -1);
	lua_getfield(l, table, "text_layer");
	desc->text_layer = luaL_checkinteger(l, -1);
	_getrectfield(l, table, "src_button_down", desc->src_button_down);
	_getrectfield(l, table, "src_button_up", desc->src_button_up);
	_getrectfield(l, table, "src_switch_on_up", desc->src_switch_on_up);
	_getrectfield(l, table, "src_switch_on_down", desc->src_switch_on_down);
	_getrectfield(l, table, "src_switch_off_up", desc->src_switch_off_up);
	_getrectfield(l, table, "src_switch_off_down", desc->src_switch_off_down);
	_getrectfield(l, table, "src_slider", desc->src_slider);
	_getrectfield(l, table, "src_slider_knob_up", desc->src_slider_knob_up);
	_getrectfield(l, table, "src_slider_knob_down", desc->src_slider_knob_down);
	return true;
}

static int ml_gui_default_style(lua_State* l) {
	checkargs(1, "gui.default_style");
	const char* prefix = luaL_checkstring(l, 1);
	GuiDesc desc = gui_default_style(prefix);
	_make_guidesc(l, &desc);
	return 1;
}

static int ml_gui_init(lua_State* l) {
	checkargs(1, "gui.init");
	GuiDesc desc;
	if(!_read_guidesc(l, &desc))
		return luaL_error(l,"bad guidesc");
	gui_init(&desc);
	return 0;
}

static int ml_gui_close(lua_State* l) {
	checkargs(0, "gui.close");
	gui_close();
	return 0;
}

static int ml_gui_label(lua_State* l) {
	checkargs(2, "gui.label");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l,"bad pos");
	const char* text = luaL_checkstring(l, 2);
	gui_label(&pos, text);
	return 0;
}

static int ml_gui_button(lua_State* l) {
	checkargs(2, "gui.button");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l,"bad pos");
	const char* text = luaL_checkstring(l, 2);
	lua_pushboolean(l, gui_button(&pos, text));
	return 1;
}

static int ml_gui_switch(lua_State* l) {
	checkargs(2, "gui.switch");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l,"bad pos");
	const char* text = luaL_checkstring(l, 2);
	lua_pushboolean(l, gui_switch(&pos, text));
	return 1;
}

static int ml_gui_slider(lua_State* l) {
	checkargs(1, "gui.slider");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l, "bad pos");
	lua_pushnumber(l, gui_slider(&pos));
	return 1;
}

static int ml_gui_switch_state(lua_State* l) {
	checkargs(1, "gui.switch_state");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l, "bad pos");
	lua_pushboolean(l, gui_getstate_switch(&pos));
	return 1;
}

static int ml_gui_switch_set_state(lua_State* l) {
	checkargs(2, "gui.switch_set_state");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l, "bad pos");
	bool state = lua_toboolean(l, 2);
	gui_setstate_switch(&pos, state);
	return 0;
}

static int ml_gui_slider_state(lua_State* l) {
	checkargs(1, "gui.slider_state");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l, "bad pos");
	lua_pushnumber(l, gui_getstate_slider(&pos));
	return 1;
}

static int ml_gui_slider_set_state(lua_State* l) {
	checkargs(2, "gui.slider_set_state");
	Vector2 pos;
	if(!_check_vec2(l, 1, &pos))
		return luaL_error(l, "bad pos");
	double state = luaL_checknumber(l, 2);
	gui_setstate_slider(&pos, state);
	return 0;
}

static const luaL_Reg gui_fun[] = {
	{"default_style", ml_gui_default_style},
	{"init", ml_gui_init},
	{"close", ml_gui_close},
	{"label", ml_gui_label},
	{"button", ml_gui_button},
	{"switch", ml_gui_switch},
	{"slider", ml_gui_slider},
	{"switch_state", ml_gui_switch_state},
	{"switch_set_state", ml_gui_switch_set_state},
	{"slider_state", ml_gui_slider_state},
	{"slider_set_state", ml_gui_slider_set_state},
	{NULL, NULL}
};


// particles

static int ml_particles_init(lua_State* l) {
	checkargs(2, "particles.init");
	const char* prefix = luaL_checkstring(l, 1);
	uint layer = luaL_checkinteger(l, 2);
	if(layer > 15)
		return luaL_error(l, "bad layer");
	particles_init(prefix, layer);
	return 0;
}

static int ml_particles_close(lua_State* l) {
	checkargs(0, "particles.close");
	particles_close();
	return 0;
}

static int ml_particles_spawn(lua_State* l) {
	int n = lua_gettop(l);
	double dir = 0.0;
	if(n == 3) {
		dir = luaL_checknumber(l, 3);
		n--;
	}

	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to particles.spawn");

	const char* name = luaL_checkstring(l, 1);
	Vector2 pos;
	if(!_check_vec2(l, 2, &pos))
		return luaL_error(l, "bad pos");
	
	particles_spawn(name, &pos, dir);
	return 0;
}

static int ml_particles_update(lua_State* l) {
	int n = lua_gettop(l);
	double time = time_ms() / 1000.0;
	if(n == 1) {
		time = luaL_checknumber(l, 1);
		n--;
	}

	if(n != 0)
		return luaL_error(l, "wrong number of arguments provided to particles.update");
	
	particles_update(time);
	return 0;
}

static int ml_particles_draw(lua_State* l) {
	checkargs(0, "particles.draw");
	particles_draw();
	return 0;
}

static const luaL_Reg particles_fun[] = {
	{"init", ml_particles_init},
	{"close", ml_particles_close},
	{"spawn", ml_particles_spawn},
	{"update", ml_particles_update},
	{"draw", ml_particles_draw},
	{NULL, NULL}
};	


// tilemap

#define checktmaphandle(l, i) \
	(Tilemap**)luaL_checkudata(l, i, "_TilemapHandle.mt")

void _new_tmaphandle(lua_State* l, Tilemap* tm) {
	Tilemap** t = (Tilemap**)lua_newuserdata(l, sizeof(Tilemap*));
	*t = tm;
	luaL_getmetatable(l, "_TilemapHandle.mt");
	lua_setmetatable(l, -2);
}

static int ml_tilemap_load(lua_State* l) {
	checkargs(1, "tilemap.load");
	const char* filename = luaL_checkstring(l, 1);
	Tilemap* tmap = tilemap_load(filename);
	_new_tmaphandle(l, tmap);
	return 1;
}

static int ml_tilemap_new(lua_State* l) {
	checkargs(5, "tilemap.new");

	uint tile_width = luaL_checkinteger(l, 1);
	uint tile_height = luaL_checkinteger(l, 2);
	uint width = luaL_checkinteger(l, 3);
	uint height = luaL_checkinteger(l, 4);
	uint layers = luaL_checkinteger(l, 5);

	Tilemap* t = tilemap_new(
		tile_width, tile_height,
		width, height, layers
	);

	_new_tmaphandle(l, t);
	return 1;
}

static int ml_tilemap_free(lua_State* l) {
	checkargs(1, "tilemap.free");
	Tilemap** t = checktmaphandle(l, 1);
	tilemap_free(*t);
	return 0;
}

static int ml_tilemap_set_tileset(lua_State* l) {
	checkargs(3, "tilemap.set_tileset");
	Tilemap** t = checktmaphandle(l, 1);
	Tilemap* tmap = *t;
	uint i = luaL_checkinteger(l, 2);
	TexHandle* ptex = checktexhandle(l, 3);

	// Alloc/realloc space for defs if needed
	if(i >= tmap->n_tilesets) {
		tmap->tilesets = tmap->n_tilesets ?
			MEM_REALLOC(tmap->tilesets, (i+1) * sizeof(TilesetDef)) :
			MEM_ALLOC((i+1) * sizeof(TilesetDef));
		tmap->n_tilesets = i+1;
	}

	TilesetDef* def = &tmap->tilesets[i];
	def->texture = *ptex;
	uint w, h;
	tex_size(*ptex, &w, &h);
	def->width = w / tmap->tile_width;
	def->height = h / tmap->tile_height;
	def->n_animdefs = 0;
	def->animdefs = NULL;

	return 0;
}

static int ml_tilemap_collision(lua_State* l) {
	checkargs(3, "tilemap.collision");
	Tilemap** t = checktmaphandle(l, 1);
	Tilemap* tmap = *t;
	int x = luaL_checkinteger(l, 2);
	int y = luaL_checkinteger(l, 3);

	lua_pushboolean(l, tilemap_is_solid(tmap, x, y));

	return 1;
}

static int ml_tilemap_set_collision(lua_State* l) {
	checkargs(4, "tilemap.set_collision");
	Tilemap** t = checktmaphandle(l, 1);
	Tilemap* tmap = *t;
	int x = luaL_checkinteger(l, 2);
	int y = luaL_checkinteger(l, 3);
	assert(x >= 0 && x < tmap->width);
	assert(y >= 0 && y < tmap->height);
	bool full = lua_toboolean(l, 4);

	uint tile = IDX_2D(x, y, tmap->width);	

	byte mask = (1 << (7-(tile % 8)));
	if(full)
		tmap->collision[tile/8] |= mask;  
	else
		tmap->collision[tile/8] &= ~mask;

	return 0;
}

static int ml_tilemap_tile(lua_State* l) {
	checkargs(4, "tilemap.tile");
	Tilemap** t = checktmaphandle(l, 1);
	Tilemap* tmap = *t;
	int x = luaL_checkinteger(l, 2);
	int y = luaL_checkinteger(l, 3);
	int layer = luaL_checkinteger(l, 4);
	
	assert(x >= 0 && x < tmap->width);
	assert(y >= 0 && y < tmap->height);
	assert(layer >= 0 && layer < tmap->n_layers);

	TilemapLayer* tl = &tmap->layers[layer];
	uint idx = IDX_2D(x, y, tmap->width);
	uint tileset = tl->data[idx] / 1024;
	uint tile = tl->data[idx] % 1024;

	lua_pushinteger(l, tileset);
	lua_pushinteger(l, tile);

	return 2;
}

static int ml_tilemap_set_tile(lua_State* l) {
	checkargs(6, "tilemap.set_tile");
	Tilemap** t = checktmaphandle(l, 1);
	Tilemap* tmap = *t;
	int x = luaL_checkinteger(l, 2);
	int y = luaL_checkinteger(l, 3);
	int layer = luaL_checkinteger(l, 4);
	int tileset = luaL_checkinteger(l, 5);
	int tile = luaL_checkinteger(l, 6);
		
	assert(x >= 0 && x < tmap->width);
	assert(y >= 0 && y < tmap->height);
	assert(layer >= 0 && layer < tmap->n_layers);
	assert(tileset < tmap->n_tilesets);
	assert(tile < 1024);

	TilemapLayer* tl = &tmap->layers[layer];
	uint idx = IDX_2D(x, y, tmap->width);
	tl->data[idx] = tileset * 1024 + tile;

	return 0;
}

static int ml_tilemap_camera(lua_State* l) {
	checkargs(1, "tilemap.camera");
	Tilemap** t = checktmaphandle(l, 1);
	Vector2 v = (*t)->camera.center;
	_new_vec2(l, v.x, v.y);
	lua_pushnumber(l, (*t)->camera.z);
	lua_pushnumber(l, (*t)->camera.rot);
	return 3;
}

static int ml_tilemap_set_camera(lua_State* l) {
	int n = lua_gettop(l);
	if(n < 2 || n > 4)
		return luaL_error(l, "wrong number of arguments provided to tilemap.set_camera");
	Tilemap** t = checktmaphandle(l, 1);
	Vector2 pos;
	if(!_check_vec2(l, 2, &pos))
		return luaL_error(l, "bad pos");
	(*t)->camera.center = pos;
	if(n >= 3) {
		double z = luaL_checknumber(l, 3);
		(*t)->camera.z = z;
	}
	if(n == 4) {
		double rot = luaL_checknumber(l, 4);
		(*t)->camera.rot = rot;
	}
	return 0;
}

static int ml_tilemap_objects(lua_State* l) {
	checkargs(1, "tilemap.objects");
	Tilemap** t = checktmaphandle(l, 1);
	int n_obj = (*t)->n_objects;
	lua_createtable(l, n_obj, 0);
	for(int i = 0; i < n_obj; ++i) {
		lua_createtable(l, 0, 2);
		Vector2 pos = (*t)->objects[i].p;
		_new_vec2(l, pos.x, pos.y);
		lua_setfield(l, -2, "pos");
		lua_pushinteger(l, (*t)->objects[i].id);
		lua_setfield(l, -2, "id");
		lua_rawseti(l, -2, i+1);	
	}
	return 1;
}

static int ml_tilemap_render(lua_State* l) {
	int n = lua_gettop(l);
	float time = time_ms() / 1000.0f;
	if(n == 3) {
		time = luaL_checknumber(l, 3);
		n--;
	}

	if(n != 2)
		return luaL_error(l, "wrong number of arguments provided to tilemap.render");
	
	Tilemap** t = checktmaphandle(l, 1);
	RectF viewport;
	if(!_check_rect(l, 2, &viewport))
		return luaL_error(l, "bad viewport");

	tilemap_render(*t, viewport, time);

	return 0;
}

static int ml_tilemap_collide(lua_State* l) {
	checkargs(2, "tilemap.collide");
	Tilemap** t = checktmaphandle(l, 1);
	Vector2 point;
	if(!_check_vec2(l, 2, &point)) {
		RectF rect;
		if(!_check_rect(l, 2, &rect))
			return luaL_error(l, "bad point/rect");
		lua_pushboolean(l, tilemap_collide(*t, rect));
		return 1;
	}
	lua_pushboolean(l, tilemap_collide_point(*t, point));
	return 1;
}

static int ml_tilemap_raycast(lua_State* l) {
	checkargs(3, "tilemap.raycast");
	Tilemap** t = checktmaphandle(l, 1);
	Vector2 start, end;
	if(!_check_vec2(l, 2, &start) || !_check_vec2(l, 3, &end))
		return luaL_error(l, "bad start/end");
	Vector2 res = tilemap_raycast(*t, start, end);
	_new_vec2(l, res.x, res.y);
	return 1;
}

static int ml_tilemap_collide_swept(lua_State* l) {
	checkargs(3, "tilemap.collide_swept");
	Tilemap** t = checktmaphandle(l, 1);
	RectF rect;
	Vector2 offset;
	if(!_check_rect(l, 2, &rect) || !_check_vec2(l, 3, &offset))
		return luaL_error(l, "bad rect/offset");
	Vector2 res = tilemap_collide_swept_rectf(*t, rect, offset);
	_new_vec2(l, res.x, res.y);
	return 1;
}

static int ml_tilemap_world2screen(lua_State* l) {
	checkargs(3, "tilemap.world2screen");
	Tilemap** t = checktmaphandle(l, 1);
	RectF viewport;
	if(!_check_rect(l, 2, &viewport))
		return luaL_error(l, "bad viewport");
	Vector2 point;
	if(!_check_vec2(l, 3, &point)) {
		RectF rect;
		if(!_check_rect(l, 3, &rect)) {
			return luaL_error(l, "bad rect/point");
		}
		RectF res = tilemap_world2screen(*t, &viewport, rect);
		_push_rect(l, res);
		return 1;
	}
	Vector2 res = tilemap_world2screen_point(*t, &viewport, point);
	_new_vec2(l, res.x, res.y);
	return 1;
}

static int ml_tilemap_screen2world(lua_State* l) {
	checkargs(3, "tilemap.screen2world");
	Tilemap** t = checktmaphandle(l, 1);
	RectF viewport;
	if(!_check_rect(l, 2, &viewport))
		return luaL_error(l, "bad viewport");
	Vector2 point;
	if(!_check_vec2(l, 3, &point)) {
		RectF rect;
		if(!_check_rect(l, 3, &rect)) {
			return luaL_error(l, "bad rect/point");
		}
		RectF res = tilemap_screen2world(*t, &viewport, rect);
		_push_rect(l, res);
		return 1;
	}
	Vector2 res = tilemap_screen2world_point(*t, &viewport, point);
	_new_vec2(l, res.x, res.y);
	return 1;
}

static const luaL_Reg tilemap_fun[] = {
	{"load", ml_tilemap_load},
	{"new", ml_tilemap_new},
	{"free", ml_tilemap_free},
	{"set_tileset", ml_tilemap_set_tileset},
	{"collision", ml_tilemap_collision},
	{"set_collision",  ml_tilemap_set_collision},
	{"tile", ml_tilemap_tile},
	{"set_tile", ml_tilemap_set_tile},
	{"camera", ml_tilemap_camera},
	{"set_camera", ml_tilemap_set_camera},
	{"objects", ml_tilemap_objects},
	{"render", ml_tilemap_render},
	{"collide", ml_tilemap_collide},
	{"raycast", ml_tilemap_raycast},
	{"collide_swept", ml_tilemap_collide_swept},
	{"world2screen", ml_tilemap_world2screen},
	{"screen2world", ml_tilemap_screen2world},
	{NULL, NULL}
};	

int malka_open_system(lua_State* l) {
	malka_register(l, "time", time_fun);

	luaL_newmetatable(l, "_TexHandle.mt");
	malka_register(l, "tex", tex_fun);

	luaL_newmetatable(l, "_FontHandle.mt");
	malka_register(l, "font", font_fun);

#ifdef __APPLE__
	malka_register(l, "vfont", vfont_fun);
#endif

	malka_register(l, "video", video_fun);

	luaL_newmetatable(l, "_SoundHandle.mt");
	luaL_newmetatable(l, "_SourceHandle.mt"); 
	malka_register(l, "sound", sound_fun);

	malka_register(l, "key", key_fun);
	malka_register(l, "char", char_fun);
	malka_register(l, "mouse", mouse_fun);
	malka_register(l, "touch", touch_fun);
	malka_register(l, "txtinput", txtinput_fun);

	lua_getglobal(l, "key");
	int tbl = lua_gettop(l);
	for(int i = 0; i < ARRAY_SIZE(key_names); ++i) {
		lua_pushinteger(l, i);
		lua_setfield(l, tbl, key_names[i]);
	}

	lua_getglobal(l, "mouse");
	tbl = lua_gettop(l);
	for(int i = 0; i < ARRAY_SIZE(mbtn_names); ++i) {
		lua_pushinteger(l, i);
		lua_setfield(l, tbl, mbtn_names[i]);
	}

	lua_pop(l, 2);
	
	malka_register(l, "orientation", orientation_fun);
		
	lua_getglobal(l, "orientation");
	tbl = lua_gettop(l);
	lua_pushinteger(l, ORIENT_LANDSCAPE_LEFT);
	lua_setfield(l, tbl, "landscape_left");
	lua_pushinteger(l, ORIENT_LANDSCAPE_RIGHT);
	lua_setfield(l, tbl, "landscale_right");
	lua_pushinteger(l, ORIENT_PORTRAIT);
	lua_setfield(l, tbl, "portrait");
	lua_pushinteger(l, ORIENT_PORTRAIT_UPSIDE_DOWN);
	lua_setfield(l, tbl, "portrait_upside_down");

	malka_register(l, "acceleration", acceleration_fun);
	malka_register(l, "runstate", runstate_fun);
	malka_register(l, "gui", gui_fun);
	malka_register(l, "particles", particles_fun);

	luaL_newmetatable(l, "_TilemapHandle.mt");
	malka_register(l, "tilemap", tilemap_fun);

	lua_pop(l, 1);

	return 1;
}
