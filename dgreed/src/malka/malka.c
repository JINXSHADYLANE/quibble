#include "malka.h"

#include "profiler.h"
#include "ml_states.h"
#include "memory.h"
#include "mempool.h"
#include "system.h"

#include <stdarg.h>
#ifdef MACOSX_BUNDLE
#include <unistd.h>
#endif

#include "lua/ltable.h"
#include "lua/lstate.h"

static lua_State* l;
static int ml_argc = 0;
static const char** ml_argv = NULL;
static bool ml_prepped = false;

extern bool fs_devmode;
static bool profiling = false;

static MemPool table_pool;
static MemPool vector_pool;
static MemPool rect_pool;
static bool pools_allocated = false;

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

extern int luaopen_bit(lua_State* l);

static void* malka_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    if (osize == 0 && nsize != 0) {
        if(nsize == sizeof(Table))
            return mempool_alloc(&table_pool);
        if(nsize == sizeof(Node) * 2)
            return mempool_alloc(&vector_pool);
        if(nsize == sizeof(Node) * 4)
            return mempool_alloc(&rect_pool);
    }    
    if (nsize == 0) {
        if(osize == sizeof(Table) && mempool_owner(&table_pool, ptr))
            mempool_free(&table_pool, ptr);
        else if(osize == sizeof(Node) * 2 && mempool_owner(&vector_pool, ptr))
            mempool_free(&vector_pool, ptr);
        else if(osize == sizeof(Node) * 4 && mempool_owner(&rect_pool, ptr))
            mempool_free(&rect_pool, ptr);
        else
            free(ptr);
        return NULL;
    }
    else {
		MemPool* pool = NULL;
        if(osize == sizeof(Table) && mempool_owner(&table_pool, ptr)) {
			pool = &table_pool;
			goto promote;
		}
		if(osize == sizeof(Node) * 2 && mempool_owner(&vector_pool, ptr)) {
			pool = &vector_pool;
			goto promote;
		}
		if(osize == sizeof(Node) * 4 && mempool_owner(&rect_pool, ptr)) {
			pool = &rect_pool;
			goto promote;
		}

        return realloc(ptr, nsize);

promote:
		// Promote pool allocated chunk to heap
		assert(pool);
		void* new = malloc(nsize);
		memcpy(new, ptr, MIN(osize, nsize));
		mempool_free(pool, ptr);
		return new;
    }
}

static int malka_panic (lua_State *L) {
    (void)L;  /* to avoid warnings */
    fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
            lua_tostring(L, -1));
    return 0;
}


lua_State* malka_newstate (void) {
    lua_State *L = lua_newstate(malka_alloc, NULL);
    if (L) lua_atpanic(L, malka_panic);
    return L;
}

void malka_init(void) {
	malka_init_ex(false);
}

// Forward defs for subsystems
extern int malka_open_vec2(lua_State* l);
extern int malka_open_rect(lua_State* l);
extern int malka_open_colors(lua_State* l);
extern int malka_open_misc(lua_State* l);
extern int malka_open_rand(lua_State* l);
extern int malka_open_log(lua_State* l);
extern int malka_open_file(lua_State* l);
extern int malka_open_mml(lua_State* l);
extern int malka_open_system(lua_State* l);
extern int malka_open_sprsheet(lua_State* l);
extern int malka_open_mfx(lua_State* l);
extern int malka_open_coldet(lua_State* l);
extern int malka_open_keyval(lua_State* l);
extern int malka_open_gamecenter(lua_State* l);
extern int malka_open_os(lua_State* l);
extern int malka_open_iap(lua_State* l);
extern int malka_open_http(lua_State* l);
extern int malka_open_localization(lua_State* l);
extern int malka_open_anim(lua_State* l);

void malka_init_ex(bool use_pools) {
	if(use_pools) {
		mempool_init_ex(&table_pool, sizeof(Table), 128*1024);
		mempool_init_ex(&vector_pool, sizeof(Node)*2, 128*1024);
		mempool_init_ex(&rect_pool, sizeof(Node)*4, 256*1024);
		
		l = malka_newstate();
		pools_allocated = true;
	}
	else {
		l = luaL_newstate();
        lua_atpanic(l, malka_panic);
	}

	luaL_openlibs(l);
	luaopen_bit(l);

	malka_open_vec2(l);
	malka_open_rect(l);
	malka_open_colors(l);
	malka_open_misc(l);
	malka_open_rand(l);
	malka_open_log(l);
	malka_open_file(l);
	malka_open_mml(l);
	malka_open_states(l);
	malka_open_system(l);
	malka_open_sprsheet(l);
	malka_open_mfx(l);
	malka_open_coldet(l);
	malka_open_keyval(l);
	malka_open_gamecenter(l);
	malka_open_os(l);
	malka_open_iap(l);
	malka_open_http(l);
	malka_open_localization(l);
	malka_open_anim(l);

	ml_states_init(l);
}

void malka_params(int argc, const char** argv) {
	ml_argc = argc;
	ml_argv = argv;
}

void malka_close(void) {
	ml_states_close(l);

	if(profiling)
		profiler_close(l);

	lua_close(l);
    
	if(pools_allocated) {
		mempool_drain(&rect_pool);
		mempool_drain(&vector_pool);
		mempool_drain(&table_pool);
		pools_allocated = false;
	}
}

int malka_register(bind_fun_ptr fun) {
	return (*fun)(l);
}

static void _malka_prep(const char* luafile) {
	// If we're in Mac OS X bundle, some rituals need to be performed
	#ifdef MACOSX_BUNDLE
	char* real_path = path_to_resource(luafile);
	char* real_folder = path_get_folder(real_path);
	
	// figure path to bundle resources
#ifdef TARGET_IOS
    const char* res_folder = "app/";
#else
    const char* res_folder = "Resources/";
#endif
	while(!_endswith(real_folder, res_folder)) {
		if(!_stripdir(real_folder))
			LOG_ERROR("Something horrible happened while trying to figure out"
				"where bundle resources are kept!");
	}

	// chdir there
	chdir(real_folder);

	MEM_FREE(real_path);
	MEM_FREE(real_folder);
	#endif

	// Register module path
	char* module_path = path_get_folder(luafile);
	lua_getglobal(l, "package"); 
	int package = lua_gettop(l);
	lua_pushfstring(l, "./%s?.lc;./%s?.lua", module_path, module_path);
	lua_setfield(l, package, "path");
	lua_pop(l, 1);
	MEM_FREE(module_path);

    // Set flag running_on_ios
#ifdef TARGET_IOS
    lua_pushboolean(l, true);
    lua_setglobal(l, "running_on_ios"); 
#endif

    // Set debug flag
#ifdef _DEBUG
    lua_pushboolean(l, true);
    lua_setglobal(l, "_DEBUG");
#endif

	// Register params
	if(ml_argv) {
		lua_createtable(l, ml_argc, 0);
		int t = lua_gettop(l);
		for(int i = 0; i < ml_argc; ++i) {
			if(strcmp(ml_argv[i], "-fsdev") == 0)
				fs_devmode = true;
			if(strcmp(ml_argv[i], "-profile") == 0)
				profiling = true;
			lua_pushstring(l, ml_argv[i]);
			lua_rawseti(l, t, i+1);
		}
	}
	else {
		lua_pushnil(l);
	}
	lua_setfield(l, LUA_GLOBALSINDEX, "argv");

	if(profiling)
		profiler_init(l);

	ml_prepped = true;
}

int malka_run_ex(const char* luafile) {
	assert(luafile);

	char* file = path_change_ext(luafile, "lc");
	if(!file_exists(file)) {
		MEM_FREE(file);
		file = strclone(luafile);
	}

	_malka_prep(file);

	if(luaL_dofile(l, file)) {
		const char* err = luaL_checkstring(l, -1);
		LOG_WARNING("error in lua script:\n%s\n", err);
		printf("An error occured:\n%s\n", err);
	}

	MEM_FREE(file);

	return 0;
}

void malka_states_init(const char* luafile) {
	malka_run_ex(luafile);
	malka_states_postinit();
}

void malka_states_postinit(void) {
	lua_getglobal(l, "game_init");
	if(lua_isfunction(l, -1))
		lua_call(l, 0, 0);
	else
		lua_pop(l, 1);
}

void malka_states_close(void) {
	lua_getglobal(l, "game_close");
	if(lua_isfunction(l, -1))
		lua_call(l, 0, 0);
	else
		lua_pop(l, 1);
}

int malka_states_run(const char* luafile) {
	malka_states_init(luafile);
	ml_states_run(l);
	malka_states_close();
	return 0;
}

double malka_call(const char* func_name, const char* fmt, ...) {
	uint n = strlen(fmt);

	lua_getglobal(l, func_name);
	if(!lua_isfunction(l, -1))
		LOG_ERROR("Unable to call %s - no such function", func_name);

	va_list v;
	va_start(v, fmt);
	for(uint i = 0; i < n; ++i) {
        const char* str;
        uint u;
        int d;
        double f;
		char t = fmt[i];
		switch(t) {
			case 's': {
				  str = va_arg(v, char*);
				  lua_pushstring(l, str);
				  break;
            }
			case 'u':
				  u = va_arg(v, uint);
				  lua_pushinteger(l, u);
				  break;
			case 'd':
				  d = va_arg(v, int);
				  lua_pushinteger(l, d);
				  break;
			case 'f':
				  f = va_arg(v, double);
				  lua_pushnumber(l, f);
				  break;
			default:
				  LOG_ERROR("Unknown malka_call fmt type %c", t);
		}
	}
	va_end(v);

	lua_call(l, n, 1);
	double res = lua_tonumber(l, -1);
	lua_pop(l, 1);

	return res;
}

void malka_gc(uint ms) {
	uint t = time_ms_current();
	//lua_gc(l, LUA_GCSETSTEPMUL, 40);

	uint before_mem, after_mem;
	do {
		before_mem = l->l_G->totalbytes;
		lua_gc(l, LUA_GCSTEP, 0);	
		after_mem = l->l_G->totalbytes;
	} while(after_mem < before_mem && time_ms_current() < t + ms);
}

void malka_full_gc(void) {
    lua_gc(l, LUA_GCCOLLECT, 0);
}

lua_State* malka_lua_state(void) {
	return l;
}

