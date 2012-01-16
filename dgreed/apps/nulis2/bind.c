#include "bind.h"

#include "sim.h"

#include <malka/ml_common.h>

static int csim_render(lua_State* l) {
	checkargs(0, "csim.render");
	sim_render();
	return 0;
}

static const luaL_Reg csim_fun[] = {
	{"render", csim_render},
	{NULL, NULL}
};

int bind_open_nulis2(lua_State* l) {
	luaL_register(l, "csim", csim_fun);
	return 1;
}
