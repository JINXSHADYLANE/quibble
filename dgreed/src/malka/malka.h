#ifndef MALKA_H
#define MALKA_H

#include <utils.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

// Basic API:
int malka_run(const char* luafile);

// Extended API:

typedef int (*bind_fun_ptr)(lua_State* l);

// Basic init/close, must be called if you're not using basic API
void malka_init(void);
void malka_close(void);

// Call this to expose params to lua code, also handle -fsdev
void malka_params(int argc, const char** argv);
// Binds some of your code to lua
int malka_register(bind_fun_ptr fun);
// Runs specified file
int malka_run_ex(const char* luafile);
// Enters ml_states controlled game loop
int malka_states_run(const char* luafile);

// Alternatives for writing your own game loop
void malka_states_init(const char* luafile);
void malka_states_close(void);
extern bool malka_states_step(void);

#endif
