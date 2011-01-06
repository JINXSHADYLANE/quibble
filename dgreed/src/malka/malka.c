#include "malka.h"

#include "ml_utils.h"
#include "ml_system.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

int malka_run(const char* luafile) {
	log_init("malka.log", LOG_LEVEL_INFO);

	lua_State* l = luaL_newstate();
	luaL_openlibs(l);
	malka_open_vec2(l);
	malka_open_rect(l);
	malka_open_colors(l);
	malka_open_misc(l);
	malka_open_rand(l);
	malka_open_log(l);
	malka_open_file(l);
	malka_open_system(l);

	if(luaL_dofile(l, luafile)) {
		const char* err = luaL_checkstring(l, -1);
		printf("An error occured:\n%s\n", err);
	}

	lua_close(l);
	log_close();
	return 0;
}

