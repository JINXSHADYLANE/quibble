#ifndef ML_UTILS_H
#define ML_UTILS_H

#include "lua/lua.h"

int malka_open_vec2(lua_State* l);
int malka_open_rect(lua_State* l);
int malka_open_colors(lua_State* l);
int malka_open_misc(lua_State* l);
int malka_open_rand(lua_State* l);
int malka_open_log(lua_State* l);
int malka_open_file(lua_State* l);

#endif
