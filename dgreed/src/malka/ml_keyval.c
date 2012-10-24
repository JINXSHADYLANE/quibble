#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <keyval.h>

static int ml_keyval_init(lua_State* l) {
	checkargs(1, "keyval.init");

	const char* filename = luaL_checkstring(l, 1);

	keyval_init(filename);

	return 0;
}

static int ml_keyval_close(lua_State* l) {
	checkargs(0, "keyval.close");

	keyval_close();

	return 0;
}

static int ml_keyval_get(lua_State* l) {
	checkargs(2, "keyval.get");

	const char* key = luaL_checkstring(l, 1);

	if(lua_isnumber(l, 2)) {
		float def = lua_tonumber(l, 2);
		lua_pushnumber(l, keyval_get_float(key, def));
		return 1;
	}

	if(lua_isboolean(l, 2)) {
		bool def = lua_toboolean(l, 2);
		lua_pushboolean(l, keyval_get_bool(key, def));
		return 1;
	}

	if(lua_isstring(l, 2)) {
		const char* def = lua_tostring(l, 2);
		lua_pushstring(l, keyval_get(key, def));
		return 1;
	}

	return luaL_error(l, "bad keyval.get default value type");
}

static int ml_keyval_set(lua_State* l) {
	checkargs(2, "keyval.set");

	const char* key = luaL_checkstring(l, 1);

	if(lua_isnumber(l, 2)) {
		float val = lua_tonumber(l, 2);
		keyval_set_float(key, val);
		return 0;
	}

	if(lua_isboolean(l, 2)) {
		bool val = lua_toboolean(l, 2);
		keyval_set_bool(key, val);
		return 0;
	}

	if(lua_isstring(l, 2)) {
		const char* val = lua_tostring(l, 2);
		keyval_set(key, val);
		return 0;
	}

	return luaL_error(l, "bad keyval.set value type");
}

static int ml_keyval_flush(lua_State* l) {
	checkargs(0, "keyval.flush");

	keyval_flush();

	return 0;
}

static int ml_keyval_gc(lua_State* l) {
	checkargs(0, "keyval.gc");

	keyval_gc();

	return 0;
}

static int ml_keyval_wipe(lua_State* l) {
	checkargs(0, "keyval.wipe");

	keyval_wipe();

	return 0;
}

static const luaL_Reg keyval_fun[] = {
	{"init", ml_keyval_init},
	{"close", ml_keyval_close},
	{"get", ml_keyval_get},
	{"set", ml_keyval_set},
	{"flush", ml_keyval_flush},
	{"gc", ml_keyval_gc},
	{"wipe", ml_keyval_wipe},
	{NULL, NULL}
};

int malka_open_keyval(lua_State* l) {
	luaL_register(l, "keyval", keyval_fun);

	lua_pop(l, 1);

	return 1;
}

