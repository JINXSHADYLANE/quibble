#include "bind.h"

#include "sim.h"
#include "levels.h"

#include <malka/ml_common.h>

static int csim_render(lua_State* l) {
	checkargs(0, "csim.render");
	sim_render();
	return 0;
}

static int csim_reset(lua_State* l) {
	checkargs(1, "csim.reset");

	const char* level_name = luaL_checkstring(l, 1);
	sim_reset(level_name);
	
	return 0;
}

static int csim_level(lua_State* l) {
	checkargs(0, "csim.level");

	lua_pushstring(l, sim_level());
	return 1;
}

static const luaL_Reg csim_fun[] = {
	{"render", csim_render},
	{"reset", csim_reset},
	{"level", csim_level},
	{NULL, NULL}
};

static int clevels_parse_ed(lua_State* l) {
	checkargs(1, "clevels.parse_ed");

	const char* desc = luaL_checkstring(l, 1);
	levels_parse_ed(desc);

	return 0;
}

static const luaL_Reg clevels_fun[] = {
	{"parse_ed", clevels_parse_ed},
	{NULL, NULL}
};

int bind_open_nulis2(lua_State* l) {
	luaL_register(l, "csim", csim_fun);
	luaL_register(l, "clevels", clevels_fun);
	return 1;
}
