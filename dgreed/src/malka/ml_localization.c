#include "ml_common.h"

#include <utils.h>
#include <localization.h>

#include "lua/lauxlib.h"
#include "lua/lualib.h"

static int ml_loc_init(lua_State* l) {
	checkargs(2, "loc.init");

	const char* files = luaL_checkstring(l, 1);
	bool production = lua_toboolean(l, 2);

	loc_init(files, production);

	return 0;
}

static int ml_loc_close(lua_State* l) {
	checkargs(0, "loc.close");

	loc_close();

	return 0;
}

static int ml_loc_select(lua_State* l) {
	checkargs(1, "loc.select");

	const char* lang = luaL_checkstring(l, 1);

	loc_select(lang);

	return 0;
}

static int ml_loc_str(lua_State* l) {
	checkargs(1, "loc.str");

	const char* str = luaL_checkstring(l, 1);
	lua_pushstring(l, loc_str(str));

	return 1;
}

static luaL_Reg loc_fun[] = {
	{"init", ml_loc_init},
	{"close", ml_loc_close},
	{"select", ml_loc_select},
	{"str", ml_loc_str},
	{NULL, NULL}
};

int malka_open_localization(lua_State* l) {
	malka_register(l, "loc", loc_fun);
	return 1;
}

