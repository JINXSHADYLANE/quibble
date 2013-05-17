#ifndef ML_STATES_H
#define ML_STATES_H

#include "lua/lua.h"
#include <stdbool.h>
#include "utils.h"

// General/lua interface
void ml_states_init(lua_State* l);
void ml_states_close(lua_State* l);
void ml_states_run(lua_State* l);
int malka_open_states(lua_State* l);

// C interface

typedef struct {
	void (*init)(void);
	void (*close)(void);
	void (*preenter)(void);
	void (*enter)(void);
	void (*leave)(void);
	void (*postleave)(void);
	const char* (*save)(void);
	void (*load)(const char* data);
	bool (*update)(void);
	bool (*render)(float);
} StateDesc;

void malka_states_register(const char* name, StateDesc* state); 
void malka_states_push(const char* name);
void malka_states_push_multi(const char** names, uint n_names);
void malka_states_replace(const char* name);
void malka_states_pop(void);
void malka_states_pop_multi(uint n);
void malka_states_set_transition_len(float len);
float malka_states_transition_len(void);

// Height of current stack
uint malka_states_count(void);
// i-th state name counting from the top of the stack (0 is top)
const char* malka_states_at(uint i);

void malka_states_save(const char* filename);
bool malka_states_load(const char* filename);

void malka_states_start(void);
void malka_states_end(void);
bool malka_states_step(void);
float malka_state_time(const char* name);

typedef void (*PreRenderCallback)(void);
void malka_states_prerender_cb(PreRenderCallback cb);

void malka_states_app_suspend(void);

#endif
