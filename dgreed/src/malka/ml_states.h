#ifndef ML_STATES_H
#define ML_STATES_H

#include "lua/lua.h"
#include <stdbool.h>

// General/lua interface
void ml_states_init(lua_State* l);
void ml_states_close(lua_State* l);
void ml_states_run(lua_State* l);
int malka_open_states(lua_State* l);

// C interface

typedef struct {
	void (*init)(void);
	void (*close)(void);
	void (*enter)(void);
	void (*leave)(void);
	bool (*update)(void);
	bool (*render)(float);
} StateDesc;

void malka_states_register(const char* name, StateDesc* state); 
void malka_states_push(const char* name);
void malka_states_replace(const char* name);
void malka_states_pop(void);
void malka_states_set_transition_len(float len);
float malka_states_transition_len(void);

bool malka_states_step(void);

#endif
