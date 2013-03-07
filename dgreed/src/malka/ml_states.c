#include "ml_states.h"
#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <system.h>
#include <memory.h>

#define max_states 32
#define max_state_name_len 16
#define max_state_stack_depth 16

typedef struct {
	const char* name;
	StateDesc desc;
} CState;

static CState c_states[max_states];
static uint n_c_states;

static char state_names[max_states][max_state_name_len];
static float states_enter_t[max_states];
static float states_acc_t[max_states];
static uint n_state_names;

static uint state_stack[max_state_stack_depth];
static uint state_stack_size;

static bool states_in_mainloop = false;
static uint states_from, states_to;
static float states_transition_t;

static PreRenderCallback pre_render_cb = NULL;

static bool _call_c_state_func(CState* state, const char* func, float* arg,
		const char* str_arg, const char** str_out) {
	assert(state && func);

	// Update & render are called most often, handle first
	if(strcmp(func, "update") == 0) {
		if(state->desc.update)
			return (*state->desc.update)();
		else
			return true;
	}

	if(strcmp(func, "render") == 0) {
		assert(arg);
		assert(*arg >= -1.0f && *arg <= 1.0f);
		if(state->desc.render)
			return (*state->desc.render)(*arg);
		else
			return true;
	}

	if(strcmp(func, "save") == 0) {
        if(!str_out) {
            LOG_WARNING("Skipped save");
            return false;
        }
		if(state->desc.save) {
			const char* data = (*state->desc.save)();
			*str_out = data;
		}
		else {
			*str_out = NULL;
		}
		return true;
	}

	if(strcmp(func, "load") == 0) {
		assert(str_arg);
		if(state->desc.load) {
			(*state->desc.load)(str_arg);
		}
		return true;
	}

	// Handle other state methods
	struct {
		const char* name;
		void (*func)(void);
	} method_tbl[] = {
		{"init", state->desc.init},
		{"close", state->desc.close},
		{"enter", state->desc.enter},
		{"leave", state->desc.leave},
		{"preenter", state->desc.preenter},
		{"postleave", state->desc.postleave}
	};

	for(uint i = 0; i < ARRAY_SIZE(method_tbl); ++i) {
		if(strcmp(method_tbl[i].name, func) == 0) {
			if(method_tbl[i].func)
				(*method_tbl[i].func)();
			return true;
		}
	}

	assert(0 && "Bad C state method name!");
	return false;
}

static bool _call_state_func(lua_State* l, const char* name, 
		const char* func, float* arg, const char* str_arg, const char** str_out) {
	assert(name && func); 

	// Handle C state
	for(uint i = 0; i < n_c_states; ++i) {
		if(strcmp(c_states[i].name, name) == 0) {
			return _call_c_state_func(&c_states[i], func, arg, NULL, NULL);
		}
	}

	// Handle lua state
	
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

	if(str_arg)
		lua_pushstring(l, str_arg);

	assert(arg == NULL || str_arg == NULL);
	
	lua_call(l, (arg || str_arg) ? 1 : 0, 1);

	bool res = lua_toboolean(l, -1);

	if(strcmp(func, "save") == 0) {
		if(lua_isstring(l, -1))
			*str_out = strclone(lua_tostring(l, -1));
	}

	lua_pop(l, 4);
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
	return state_stack_size;
}

static uint _stack_get(uint i) {
	assert(i < state_stack_size);
	return state_stack[i];
}

static void _stack_push(uint i) {
	assert(i < n_state_names);
	assert(state_stack_size < max_state_stack_depth);
	state_stack[state_stack_size++] = i;
}

static void _stack_pop(void) {
	assert(state_stack_size);
	state_stack_size--;
}

static uint _names_size(void) {
	return n_state_names;
}

static const char* _names_get(uint i) {
	assert(i < n_state_names);
	return state_names[i];
}

static void _names_add(const char* name) {
	assert(strlen(name) < max_state_name_len-1);
#ifdef _DEBUG
	// Check if name already exists
	for(uint i = 0; i < n_state_names; ++i) {
		if(strcmp(_names_get(i), name) == 0)
			LOG_ERROR("Trying to add state with already used name");
	}
#endif
	assert(n_state_names < max_states);
	strcpy(state_names[n_state_names++], name);
}

static uint _names_find(const char* name) {
	for(uint i = 0; i < n_state_names; ++i) {
		if(strcmp(name, state_names[i]) == 0) {
			return i;
		}
	}
	return ~0;
}

static int ml_states_time(lua_State* l) {
	int n = lua_gettop(l);
	if(n == 0) {
		lua_pushnumber(l, malka_state_time(NULL));
		return 1;
	}
	if(n == 1) {
		const char* name = luaL_checkstring(l, 1);
		lua_pushnumber(l, malka_state_time(name));
		return 1;
	}
	return luaL_error(l, "bad args to states.time");
}

static int ml_states_register(lua_State* l) {
	checkargs(2, "states.register");
		
	const char* name = luaL_checkstring(l, 1);
	assert(lua_istable(l, 2));

	_register_state(l, name, 2);
	_names_add(name);

	return 1;
}

static void _states_push(lua_State* l, uint idx) {
	bool stack_empty = _stack_size() == 0;	

	int top;
	if(!stack_empty) {
		// Leave old state
		top = _stack_get(_stack_size()-1);
		_call_state_func(l, _names_get(top), "leave", NULL, NULL, NULL);
		states_acc_t[top] += time_s() - states_enter_t[top];
	}

	_stack_push(idx);
	_call_state_func(l, _names_get(idx), "preenter", NULL, NULL, NULL);

	if(!stack_empty) {
		// Set up transition
		states_from = top;
		top = _stack_get(_stack_size()-1);
		states_to = top;
		states_transition_t = time_s();
	}
}

static void _states_push_multi(lua_State* l, const char** names, uint n_names) {
	bool stack_empty = _stack_size() == 0;

	int top = 0;
	if(!stack_empty) {
		top = _stack_get(_stack_size()-1);
		_call_state_func(l, _names_get(top), "leave", NULL, NULL, NULL);
		states_acc_t[top] += time_s() - states_enter_t[top];
	}

	for(uint i = 0; i < n_names; ++i) {
		uint idx = _names_find(names[i]);
		_stack_push(idx);
	}

	if(!stack_empty) {
		states_from = top;
		top = _stack_get(_stack_size()-1);
		states_to = top;
		states_transition_t = time_s();
		_call_state_func(l, _names_get(top), "preenter", NULL, NULL, NULL);
	}
}

static int ml_states_push(lua_State* l) {
	checkargs(1, "states.push");
	
	const char* name = luaL_checkstring(l, 1);

	uint idx = _names_find(name);
	if(idx == ~0)
		return luaL_error(l, "No such state!");

	_states_push(l, idx);

	return 0;
}

static int ml_states_push_multi(lua_State* l) {
	checkargs(1, "states.push_multi");
	assert(lua_istable(l, 1));

	char str_pool[256];
    char* str = str_pool;
	const char* ptrs[8] = { NULL };

	uint n_names = lua_objlen(l, 1);
	assert(n_names <= 8);

	for(uint i = 0; i < n_names; ++i) {
		lua_rawgeti(l, 1, i+1);
		size_t len;
		const char* s = lua_tolstring(l, -1, &len);
		ptrs[i] = str;
		memcpy(str, s, len+1);
		str += len+1;
		assert(str < str_pool + 256);

		lua_pop(l, 1);
	}

	_states_push_multi(l, ptrs, n_names);

	return 0;
}

static void _state_pop(lua_State* l) {
    assert(_stack_size());

	// Leave old state
	int top = _stack_get(_stack_size()-1);
	_call_state_func(l, _names_get(top), "leave", NULL, NULL, NULL);
	states_acc_t[top] += time_s() - states_enter_t[top];

	_stack_pop();

	// Set up transition
	if(_stack_size()) {
		states_from = top;
		top = _stack_get(_stack_size()-1);
		states_to = top;
		states_transition_t = time_s();
		_call_state_func(l, _names_get(top), "preenter", NULL, NULL, NULL);
	}
    else {
        LOG_WARNING("Stack was just emptied");
    }
}

static int ml_states_pop(lua_State* l) {
	checkargs(0, "states.pop");

	if(state_stack_size == 0)
		return luaL_error(l, "States stack is already empty!");

	_state_pop(l);

	return 0;
}

static void _states_replace(lua_State* l, uint idx) {
	// Leave old state
	int top = _stack_get(_stack_size()-1);
	_call_state_func(l, _names_get(top), "leave", NULL, NULL, NULL);
	_call_state_func(l, _names_get(idx), "preenter", NULL, NULL, NULL);
	states_acc_t[top] += time_s() - states_enter_t[top];

	_stack_pop();
	_stack_push(idx);

	// Set up transition
	states_from = top;
	top = _stack_get(_stack_size()-1);
	states_to = top;
	states_transition_t = time_s();
}

static int ml_states_replace(lua_State* l) {
	checkargs(1, "states.replace");

	const char* name = luaL_checkstring(l, 1);

	uint idx = _names_find(name);
	if(idx == ~0)
		return luaL_error(l, "No such state!");

	if(state_stack_size == 0)
		return luaL_error(l, "Trying to replace the top of empty stack!");

	_states_replace(l, idx);

	return 0;
}

static lua_State* cb_l = NULL;
static int cb_prerender_ref = -1;

static void _prerender_cb(void) {
	assert(cb_l);
	lua_getref(cb_l, cb_prerender_ref);
	assert(lua_isfunction(cb_l, -1));
	lua_call(cb_l, 0, 0);
}

static int ml_states_prerender_callback(lua_State* l) {
	checkargs(1, "states.prerender_callback");

	if(cb_prerender_ref != -1) {
		lua_unref(l, cb_prerender_ref);
		cb_prerender_ref = -1;
	}

	if(lua_isnil(l, 1)) {
		malka_states_prerender_cb(NULL);
	}
	else if(lua_isfunction(l, 1)) {
		cb_l = l;
		cb_prerender_ref = lua_ref(l, 1);
		malka_states_prerender_cb(_prerender_cb);
	}
	else {
		return luaL_error(l, "wrong callback type");
	}

	return 0;
}

static int ml_states_size(lua_State* l) {
	checkargs(0, "states.size");

	lua_pushinteger(l, _stack_size());
	return 1;
}

static int ml_states_top(lua_State* l) {
	checkargs(0, "states.top");

	uint top = _stack_get(_stack_size()-1);
	lua_pushstring(l, _names_get(top));

	return 1;
}

static int ml_states_at(lua_State* l) {
	checkargs(1, "states.at");

	uint i = luaL_checkinteger(l, 1);

	assert(i < _stack_size());

	uint s = _stack_get(_stack_size()-1 - i);
	lua_pushstring(l, _names_get(s));

	return 1;
}

uint malka_states_count(void) {
	return _stack_size();
}

const char* malka_states_at(uint i) {
	assert(i < _stack_size());
	uint s = _stack_get(_stack_size()-1 - i);
	return _names_get(s);
}

static int ml_states_save(lua_State* l) {
	checkargs(1, "states.save");

	const char* filename = luaL_checkstring(l, 1);

	malka_states_save(filename);

	return 0;
}

static int ml_states_load(lua_State* l) {
	checkargs(1, "states.load");

	const char* filename = luaL_checkstring(l, 1);

	lua_pushboolean(l, malka_states_load(filename));

	return 1;
}

void ml_states_init(lua_State* l) {
	n_c_states = 0;
	n_state_names = 0;
	state_stack_size = 0;
	states_in_mainloop = false;

	memset(states_enter_t, 0, sizeof(states_enter_t));
	memset(states_acc_t, 0, sizeof(states_acc_t));
}

void ml_states_close(lua_State* l) {
	if(_stack_size() != 0)
		LOG_WARNING("Closing states subsystem with non-empty stack.");
}

static const char* top_name;
static bool breakout;

void ml_states_run(lua_State* l) {
	malka_states_start();

	// Main loop
	bool cont = true;
	do {
		cont = malka_states_step();
	} while(cont);

	malka_states_end();
}

static const luaL_Reg states_fun[] = {
	{"time", ml_states_time},
	{"register", ml_states_register},
	{"push", ml_states_push},
	{"push_multi", ml_states_push_multi},
	{"pop", ml_states_pop},
	{"replace", ml_states_replace},
	{"prerender_callback", ml_states_prerender_callback},
	{"size", ml_states_size},
	{"top", ml_states_top},
	{"at", ml_states_at},
	{"save", ml_states_save},
	{"load", ml_states_load},
	{NULL, NULL}
};

int malka_open_states(lua_State* l) {
	malka_register(l, "states", states_fun);
	_set_transition_len(l, 0.0f);
	return 1;
}

void malka_states_register(const char* name, StateDesc* state) {
	assert(n_c_states < max_states);

	_names_add(name);
	c_states[n_c_states].name = name;
	c_states[n_c_states].desc = *state;
	n_c_states++;
}

extern lua_State* malka_lua_state(void);

void malka_states_push(const char* name) {
	uint idx = _names_find(name);
	assert(idx != ~0);
	_states_push(malka_lua_state(), idx);
}

void malka_states_push_multi(const char** names, int n_names) {
	_states_push_multi(malka_lua_state(), names, (uint)n_names);
}

void malka_states_replace(const char* name) {
	uint idx = _names_find(name);
	assert(idx != ~0);
	assert(state_stack_size > 0);
	_states_replace(malka_lua_state(), idx);
}

void malka_states_pop(void) {
	assert(state_stack_size > 0);
	_state_pop(malka_lua_state());
}

void malka_states_set_transition_len(float len) {
	_set_transition_len(malka_lua_state(), len);
}

float malka_states_transition_len(void) {
	return _get_transition_len(malka_lua_state());
}

void malka_states_save(const char* filename) {
	// Save format:
	// 4 - MSV0
	// 4 - n states
	// for each state:
	//  4 name length
	//  x name
	//  4 data length
	//  x data
	
	lua_State* l = malka_lua_state();
	
	FileHandle out = file_create(filename);
	file_write_uint32(out, FOURCC('M', 'S', 'V', '0'));
	file_write_uint32(out, _stack_size());
	for(uint i = 0; i < _stack_size(); ++i) {
		// state name
		const char* name = _names_get(_stack_get(i));
		assert(name);
		uint32 name_len = strlen(name);
		assert(name_len);

		file_write_uint32(out, name_len);
		file_write(out, name, name_len);

		// persistent state data
		const char* data = NULL;
		_call_state_func(l, name, "save", NULL, NULL, &data); 
		if(data) {
			uint32 data_len = strlen(data);
			file_write_uint32(out, data_len);
			file_write(out, data, data_len);
			MEM_FREE(data);
		}
		else {
			file_write_uint32(out, 0);
		}
	}

	file_close(out);
}

bool malka_states_load(const char* filename) {
	if(file_exists(filename)) {
		lua_State* l = malka_lua_state();

		FileHandle in = file_open(filename);
		if(file_read_uint32(in) != FOURCC('M', 'S', 'V', '0'))
			goto error;

		uint n = file_read_uint32(in);
		if(n < 1)
			goto error;

		char buffer[256];
		for(uint i = 0; i < n; ++i) {
			// Name
			uint32 name_len = file_read_uint32(in); 
			assert(name_len > 0 && name_len < 256);
			file_read(in, buffer, name_len);
			buffer[name_len] = '\0';
			uint name_idx = _names_find(buffer);
			assert(name_idx != ~0);
			_stack_push(name_idx);

			// Data
			uint32 data_len = file_read_uint32(in);
			assert(data_len < 256);
			if(data_len) {
				file_read(in, buffer, data_len);
			}
			buffer[data_len] = '\0';
			_call_state_func(l, _names_get(name_idx), "load", NULL, buffer, NULL);
		}

		file_close(in);
		return true;

error:
		file_close(in);
	}
	return false;
}

void malka_states_start(void) {
	lua_State* l = malka_lua_state();

	if(!_stack_size()) {
		LOG_WARNING("Entering main loop with empty state stack");
		return;
	}	

	states_from = states_to = _stack_get(_stack_size()-1);	

	// Init all states
	//for(uint i = 0; i < n_c_states; ++i) {
	//	_call_c_state_func(&c_states[i], "init", NULL, NULL, NULL);
	//}
	for(uint i = 0; i < n_state_names; ++i) {
		_call_state_func(l, state_names[i], "init", NULL, NULL, NULL);
	}

	// Enter top state
	top_name = _names_get(states_from);
	states_enter_t[states_from] = time_s();
	_call_state_func(l, top_name, "preenter", NULL, NULL, NULL);
	_call_state_func(l, top_name, "enter", NULL, NULL, NULL);

	// Prep for main loop
	states_in_mainloop = true;
	breakout = false;
}

void malka_states_end(void) {
	lua_State* l = malka_lua_state();

	states_in_mainloop = false;

	// Leave top state if stack is not empty
	if(_stack_size()) {
		top_name = _names_get(_stack_get(_stack_size()-1));
		_call_state_func(l, top_name, "leave", NULL, NULL, NULL);
		_call_state_func(l, top_name, "postleave", NULL, NULL, NULL);
	}

	for(uint i = 0; i < _names_size(); ++i)
		_call_state_func(l, _names_get(i), "close", NULL, NULL, NULL);
}

bool dgreed_sleeping = false;
bool malka_states_step(void) {
	assert(states_in_mainloop);
	lua_State* l = malka_lua_state();

	if(dgreed_sleeping) {
		// Do nothing
	}
	else if(_in_transition()) {
		float time = time_s();
		float len = _get_transition_len(l);

		if(states_transition_t + len > time + 1.0f || states_transition_t > time)
			len = 0.0f;

		if(len <= 0.0f || states_transition_t + len <= time) {
			// End transition
			states_enter_t[states_to] = time_s();
			_call_state_func(l, _names_get(states_from), "postleave", NULL, NULL, NULL);
			_call_state_func(l, _names_get(states_to), "enter", NULL, NULL, NULL);
			states_from = states_to;
		}
		else {
			// Render transition
			float t = (time - states_transition_t) / len;
			// t is never zero when transitioning
			t = clamp(0.00001f, 0.9999f, t);
			float tt = -1.0f + t;
			_call_state_func(l, _names_get(states_from), "render", &t, NULL, NULL);
			_call_state_func(l, _names_get(states_to), "render", &tt, NULL, NULL);
			if(pre_render_cb)
				(*pre_render_cb)();
			video_present();
		}
	}
	else {
		if(_stack_size()) {
			top_name = _names_get(_stack_get(_stack_size()-1));
			if(!_call_state_func(l, top_name, "update", NULL, NULL, NULL)) {
				breakout = true;
			}
			else {
				// Update may have made stack empty, check again
				if(_stack_size()) {
					float zero = 0.0f;
					breakout = !_call_state_func(l, top_name, "render", &zero, NULL, NULL);
					if(pre_render_cb)
						(*pre_render_cb)();
					video_present();
				}
			}	
		}
	}
	if(!system_update())
		breakout = true;

	return !breakout && (_stack_size() || _in_transition());
}

float malka_state_time(const char* name) {
	assert(_stack_size());

	uint top = _stack_get(_stack_size()-1);

	if(name) {
		uint idx = _names_find(name);
		float t = 0.0f;
		if(idx == top && !_in_transition())
			t = time_s() - states_enter_t[idx];
		return t + states_acc_t[idx];
	}
	else {
		return time_s() - states_enter_t[top] + states_acc_t[top];
	}
}

void malka_states_prerender_cb(PreRenderCallback cb) {
	pre_render_cb = cb;
}

void malka_states_app_suspend(void) {
	if(_stack_size()) {
		malka_states_save("states.db");
	}
}
