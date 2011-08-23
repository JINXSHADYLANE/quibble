#include "malka.h"

#include "ml_utils.h"
#include "ml_system.h"
#include "ml_mml.h"

#ifdef MACOSX_BUNDLE
#include "memory.h"
#include <unistd.h>
#endif

static lua_State* l;
static int ml_argc = 0;
static const char** ml_argv = NULL;

extern bool fs_devmode;

bool _endswith(const char* str, const char* tail) {
	assert(str && tail);

	size_t str_len = strlen(str);
	size_t tail_len = strlen(tail);
	if(tail_len > str_len)
		return false;

	while(tail_len) {
		if(str[--str_len] != tail[--tail_len])
			return false;
	}
	return true;
}

bool _stripdir(char* path) {
	assert(path);

	size_t path_len = strlen(path);
	if(path_len < 3)
		return false;
	path_len -= 2;
	while(path[path_len] != '/') path_len--;
	path[path_len+1] = '\0';
	return true;
}

int malka_run(const char* luafile) {
	malka_init();
	int res = malka_run_ex(luafile);
	malka_close();
	return res;
}

void malka_init(void) {
	l = luaL_newstate();
	luaL_openlibs(l);
	malka_open_vec2(l);
	malka_open_rect(l);
	malka_open_colors(l);
	malka_open_misc(l);
	malka_open_rand(l);
	malka_open_log(l);
	malka_open_file(l);
	malka_open_mml(l);
	malka_open_system(l);
}

void malka_params(int argc, const char** argv) {
	assert(argc && argv);

	ml_argc = argc;
	ml_argv = argv;
}

void malka_close(void) {
	lua_close(l);
}

int malka_register(bind_fun_ptr fun) {
	return (*fun)(l);
}

int malka_run_ex(const char* luafile) {
	// If we're in Mac OS X bundle, some rituals need to be performed
	#ifdef MACOSX_BUNDLE
	char* real_path = path_to_resource(luafile);
	char* real_folder = path_get_folder(real_path);
	
	// figure path to bundle resources
	while(!_endswith(real_folder, "Resources/")) {
		if(!_stripdir(real_folder))
			LOG_ERROR("Something horrible happened while trying to figure out"
				"where bundle resources are kept!");
	}

	// chdir there
	chdir(real_folder);

	MEM_FREE(real_path);
	MEM_FREE(real_folder);
	#endif

	// Register params
	if(ml_argv) {
		lua_createtable(l, ml_argc, 0);
		int t = lua_gettop(l);
		for(int i = 0; i < ml_argc; ++i) {
			if(strcmp(ml_argv[i], "-fsdev") == 0)
				fs_devmode = true;
			lua_pushstring(l, ml_argv[i]);
			lua_rawseti(l, t, i+1);
		}
	}
	else {
		lua_pushnil(l);
	}
	lua_setfield(l, LUA_GLOBALSINDEX, "argv");

	if(luaL_dofile(l, luafile)) {
		const char* err = luaL_checkstring(l, -1);
		LOG_WARNING("error in lua script:\n%s\n", err);
		printf("An error occured:\n%s\n", err);
	}

	return 0;
}

