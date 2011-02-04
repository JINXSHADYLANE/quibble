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

void malka_init(void);
int malka_register(bind_fun_ptr fun);
int malka_run_ex(const char* luafile);
void malka_close(void);

#endif
