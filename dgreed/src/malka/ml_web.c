#include "ml_web.h"
#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#ifdef TARGET_IOS

extern void ios_open_web_url(const char* url);

static int ml_web_open(lua_State* l) {
	checkargs(1, "web.open");

	const char* url = luaL_checkstring(l, 1);
	ios_open_web_url(url);
	
	return 0;
}

#else

static int ml_web_open(lua_State* l) {
	checkargs(1, "web.open");

	return 0;
}

#endif

static luaL_Reg web_fun[] = {
	{"open", ml_web_open},
	{NULL, NULL}
};

int malka_open_web(lua_State* l) {
	luaL_register(l, "web", web_fun);

	lua_pop(l, 1);

	return 1;
}
