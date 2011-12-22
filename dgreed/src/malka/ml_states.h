#ifndef ML_STATES_H
#define ML_STATES_H

#include "lua/lua.h"

void ml_states_init(lua_State* l);
void ml_states_close(lua_State* l);
void ml_states_run(lua_State* l);
int malka_open_states(lua_State* l);

#endif
