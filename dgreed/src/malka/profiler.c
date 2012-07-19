#include "profiler.h"

static void _hook(lua_State* l, lua_Debug* d) {
	lua_getinfo(l, "nS", d);
	if(strcmp(d->short_src, "[C]") != 0)
		printf("%s %s\n", d->event == LUA_HOOKCALL ? ">" : "<", d->name);
}

void profiler_init(lua_State* l) {
	assert(l);

	lua_sethook(l, _hook, LUA_MASKCALL | LUA_MASKRET, 0);
}

void profiler_close(lua_State* l) {
	lua_sethook(l, _hook, 0, 0);
}

