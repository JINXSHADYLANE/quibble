#include "malka.h"

#include "ml_utils.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

int malka_run(const char* luafile) {
	lua_State* l = luaL_newstate();
	luaL_openlibs(l);
	malka_open_vec2(l);
	malka_open_rect(l);
	malka_open_colors(l);
	malka_open_misc(l);

	if(luaL_dofile(l, luafile)) {
		const char* err = luaL_checkstring(l, -1);
		printf("An error occured:\n%s\n", err);
	}

	lua_close(l);
	return 0;
}

