#include "ml_states.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <memory.h>
#include <darray.h>
#include <system.h>

static bool states_in_mainloop = false;
static DArray states_names;
static DArray states_stack;
static uint states_from, states_to;
static float states_transition_t;

static bool _call_state_func(lua_State* l, const char* name, 
		const char* func, float* arg) {
	assert(name && func); 

	lua_getglobal(l, "states");	
	int states = lua_gettop(l);
	assert(lua_istable(l, states));
	
	lua_getfield(l, states, "storage");
	int storage = lua_gettop(l);
	assert(lua_istable(l, storage));
	
	lua_getfield(l, storage, name);
	int state = lua_gettop(l);
	if(!lua_istable(l, state))
		LOG_ERROR("Unable to call state func %s.%s! No such state.", name, func);
	
		lua_getfield(l, state, func);
	int f = lua_gettop(l);
	if(!lua_isfunction(l, f)) {
		lua_pop(l, 4);
		return true;
	}
	
	if(arg)
		lua_pushnumber(l, *arg);
	
	lua_call(l, arg ? 1 : 0, 1);

	bool res = lua_toboolean(l, -1);
	lua_pop(l, 5);
	return res;
}

static void _register_state(lua_State* l, const char* name, int state) {
	assert(name);

	lua_getglobal(l, "states");	
	int states = lua_gettop(l);
	assert(lua_istable(l, states));
	
get_storage:
	lua_getfield(l, states, "storage");
	int storage = lua_gettop(l);
	if(!lua_istable(l, storage)) {
		lua_newtable(l);
		lua_setfield(l, states, "storage");
		goto get_storage;
	}

	lua_pushvalue(l, state);
	lua_setfield(l, storage, name);

	lua_pop(l, 3);
}

#define checkargs(c, name) \
	int n = lua_gettop(l); \
	if(n != c) \
		return luaL_error(l, "wrong number of arguments provided to " name \
			"; got %d, expected " #c, n)

static float _get_transition_len(lua_State* l) {
	lua_getglobal(l, "states");	
	int states = lua_gettop(l);
	assert(lua_istable(l, states));

	lua_getfield(l, states, "transition_len");
	float len = (float)lua_tonumber(l, -1);
	lua_pop(l, 2);
	return len;
}

static void _set_transition_len(lua_State* l, float len) {
	lua_getglobal(l, "states");	
	int states = lua_gettop(l);
	assert(lua_istable(l, states));

	assert(len >= 0.0f);
	lua_pushnumber(l, len);
	lua_setfield(l, states, "transition_len");
	lua_pop(l, 1);
}

static bool _in_transition(void) {
	return states_from != states_to;
}
static uint _stack_size(void) {
	return states_stack.size;
}

static uint _stack_get(uint i) {
	uint* stack = DARRAY_DATA_PTR(states_stack, uint);
	assert(i < states_stack.size);
	return stack[i];
}

static void _stack_push(uint i) {
	assert(i < states_names.size);
	darray_append(&states_stack, (void*)&i);
}

static void _stack_pop(void) {
	assert(_stack_size());
	states_stack.size--;
}

static uint _names_size(void) {
	return states_names.size;
}

static const char* _names_get(uint i) {
	const char** names = DARRAY_DATA_PTR(states_names, const char*);
	assert(i < states_names.size);
	return names[i];
}

static void _names_add(const char* name) {
#ifdef _DEBUG
	// Check if name already exists
	for(uint i = 0; i < states_names.size; ++i) {
		if(strcmp(_names_get(i), name) == 0)
			LOG_ERROR("Trying to add state with already used name");
	}
#endif
	const char* name_clone = strclone(name);
	darray_append(&states_names, (void*)&name_clone);
}

static uint _names_find(const char* name) {
	const char** names = DARRAY_DATA_PTR(states_names, const char*);
	for(uint i = 0; i < states_names.size; ++i) {
		if(strcmp(name, names[i]) == 0) {
			return i;
		}
	}
	return ~0;
}

static int ml_states_register(lua_State* l) {
	checkargs(2, "states.register");
		
	const char* name = luaL_checkstring(l, 1);
	assert(lua_istable(l, 2));

	_register_state(l, name, 2);
	_names_add(name);

	if(states_in_mainloop)
		_call_state_func(l, name, "init", NULL);

	return 1;
}

static int ml_states_push(lua_State* l) {
	checkargs(1, "states.push");
	
	const char* name = luaL_checkstring(l, 1);

	uint idx = _names_find(name);
	if(idx == ~0)
		return luaL_error(l, "No such state!");

	// Leave old state
	int top = _stack_get(_stack_size()-1);
	_call_state_func(l, _names_get(top), "leave", NULL);

	_stack_push(idx);

	// Set up transition
	states_from = top;
	top = _stack_get(_stack_size()-1);
	states_to = top;
	states_transition_t = time_ms() / 1000.0f;

	return 0;
}

static int ml_states_pop(lua_State* l) {
	checkargs(0, "states.pop");

	if(states_stack.size == 0)
		return luaL_error(l, "States stack is already empty!");

	// Leave old state
	int top = _stack_get(_stack_size()-1);
	_call_state_func(l, _names_get(top), "leave", NULL);

	_stack_pop();

	// Set up transition
	states_from = top;
	top = _stack_get(_stack_size()-1);
	states_to = top;
	states_transition_t = time_ms() / 1000.0f;

	return 0;
}

void ml_states_init(lua_State* l) {
	states_names = darray_create(sizeof(const char*), 0);	
	states_stack = darray_create(sizeof(uint), 0);
}

void ml_states_close(lua_State* l) {
	if(_stack_size() != 0)
		LOG_WARNING("Closing states subsystem with non-empty stack.");

	const char** names = DARRAY_DATA_PTR(states_names, const char*);
	for(uint i = 0; i < states_names.size; ++i)
		MEM_FREE(names[i]);
	
	darray_free(&states_names);
	darray_free(&states_stack);
}

void ml_states_run(lua_State* l) {
	if(!_stack_size()) {
		LOG_WARNING("Entering main loop with empty state stack");
		return;
	}	

	// Init all states
	for(uint i = 0; i < _names_size(); ++i)
		_call_state_func(l, _names_get(i), "init", NULL);

	states_from = states_to = _stack_get(_stack_size()-1);	

	// Enter top state
	const char* top_name = _names_get(_stack_get(_stack_size()-1));
	_call_state_func(l, top_name, "enter", NULL);

	// Main loop
	states_in_mainloop = true;
	bool breakout = false;
	float zero = 0.0f;
	do {
		if(_in_transition()) {
			float time_s = time_ms() / 1000.0f;
			float len = _get_transition_len(l);

			if(states_transition_t + len < time_s) {
				// End transition
				_call_state_func(l, _names_get(states_to), "enter", NULL);
				states_from = states_to;
			}
			else {
				// Render transition
				float t = (time_s - states_transition_t) / len;
				float tt = -1.0f + t;
				assert(t >= 0.0f && t <= 1.0f);
				_call_state_func(l, _names_get(states_from), "render", &t);
				_call_state_func(l, _names_get(states_to), "render", &tt);
				video_present();
			}
		}
		else {
			top_name = _names_get(_stack_get(_stack_size()-1));
			if(!_call_state_func(l, top_name, "update", NULL)) {
				breakout = true;
			}
			else {
				breakout = !_call_state_func(l, top_name, "render", &zero);
				video_present();
			}	
		}
	}
	while(!breakout && (_stack_size() || _in_transition()));
	states_in_mainloop = false;

	// Leave top state if stack is not empty
	if(_stack_size()) {
		top_name = _names_get(_stack_get(_stack_size()-1));
		_call_state_func(l, top_name, "leave", NULL);
	}

	for(uint i = 0; i < _names_size(); ++i)
		_call_state_func(l, _names_get(i), "close", NULL);
}

static const luaL_Reg states_fun[] = {
	{"register", ml_states_register},
	{"push", ml_states_push},
	{"pop", ml_states_pop},
	{NULL, NULL}
};

int malka_open_states(lua_State* l) {
	luaL_register(l, "states", states_fun);
	_set_transition_len(l, 0.0f);

	return 1;
}
