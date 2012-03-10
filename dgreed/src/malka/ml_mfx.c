#include "ml_mfx.h"
#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <mfx.h>

extern bool _check_vec2(lua_State* l, int i, Vector2* v);

static int ml_mfx_init(lua_State* l) {
	checkargs(1, "mfx.init");
	
	const char* desc = luaL_checkstring(l, 1);

	mfx_init(desc);

	return 0;
}

static int ml_mfx_close(lua_State* l) {
	checkargs(0, "mfx.close");

	mfx_close();

	return 0;
}

static int ml_mfx_update(lua_State* l) {
	checkargs(0, "mfx.update");

	mfx_update();

	return 0;
}

static int ml_mfx_trigger(lua_State* l) {
	int n = lua_gettop(l);

	const char* name = luaL_checkstring(l, 1);
	if(n == 1) {
		mfx_trigger(name);	
	}
	else if(n == 2 || n == 3) {
		Vector2 pos;
		float dir = 0.0f;
		if(!_check_vec2(l, 2, &pos))
			goto error;
		if(n == 3)
			dir = luaL_checknumber(l, 3);
		mfx_trigger_ex(name, pos, dir);
	}

	return 0;

error:
	return luaL_error(l, "bad arguments to mfx.trigger");
}

static int ml_mfx_snd_volume(lua_State* l) {
	checkargs(0, "mfx.snd_volume");

	lua_pushnumber(l, mfx_snd_volume());

	return 1;
}

static int ml_mfx_snd_set_volume(lua_State* l) {
	checkargs(1, "mfx.snd_set_volume");

	float volume = luaL_checknumber(l, 1);
	mfx_snd_set_volume(volume);

	return 0;
}

static int ml_mfx_snd_play(lua_State* l) {
	checkargs(1, "mfx.snd_play");

	const char* name = luaL_checkstring(l, 1);
	mfx_snd_play(name);

	return 0;
}

static int ml_mfx_snd_ambient(lua_State* l) {
	checkargs(2, "mfx.snd_ambient");

	const char* name = luaL_checkstring(l, 1);
	float volume = luaL_checknumber(l, 2);
	mfx_snd_set_ambient(name, volume);

	return 0;
}

static const luaL_Reg mfx_fun[] = {
	{"init", ml_mfx_init},
	{"close", ml_mfx_close},
	{"update", ml_mfx_update},
	{"trigger", ml_mfx_trigger},
	{"snd_volume", ml_mfx_snd_volume},
	{"snd_set_volume", ml_mfx_snd_set_volume},
	{"snd_play", ml_mfx_snd_play},
	{"snd_ambient", ml_mfx_snd_ambient},
	{NULL, NULL}
};

int malka_open_mfx(lua_State* l) {
	luaL_register(l, "mfx", mfx_fun);

	lua_pop(l, 1);

	return 1;
}
