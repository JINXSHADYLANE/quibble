#ifndef PROFILER_H
#define PROFILER_H

#include <utils.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"

// Low-overhead function level lua profiler,
// measuring time & memory.

void profiler_init(lua_State* l);
void profiler_close(lua_State* l);

const char* profiler_results(void);

#endif

