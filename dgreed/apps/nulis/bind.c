#include "bind.h"

#include "effects.h"
#include "sim.h"

#include <system.h>

#define checkargs(c, name) \
	int n = lua_gettop(l); \
	if(n != c) \
		return luaL_error(l, "wrong number of arguments provided to " name \
			"; got %d, expected " #c, n)

extern bool _check_vec2(lua_State* l, int i, Vector2* v);

static int ceffects_init(lua_State* l) {
	checkargs(0, "ceffects.init");
	effects_init();
	return 0;
}

static int ceffects_close(lua_State* l) {
	checkargs(0, "ceffects.close");
	effects_close();
	return 0;
}

static int ceffects_update(lua_State* l) {
	checkargs(0, "ceffects.update");
	effects_update();
	return 0;
}

static int ceffects_render(lua_State* l) {
	checkargs(0, "ceffects.render");
	effects_render();
	return 0;
}

static int ceffects_force_field(lua_State* l) {
	checkargs(3, "ceffects.force_field");

	Vector2 p;
	if(!_check_vec2(l, 1, &p))
		return luaL_error(l, "bad first argument to ceffects.force_field");

	float r = luaL_checknumber(l, 2);
	bool push = lua_toboolean(l, 3);

	effects_force_field(p, r, push);
	return 0;
}

static int ceffects_destroy(lua_State* l) {
	checkargs(1, "ceffects.destroy");

	Vector2 p;
	if(!_check_vec2(l, 1, &p))
		return luaL_error(l, "bad first argument to ceffects.destroy");

	effects_destroy(p);
	return 0;
}

static int ceffects_spawn(lua_State* l) {
	checkargs(1, "ceffects.spawn");

	Vector2 p;
	if(!_check_vec2(l, 1, &p))
		return luaL_error(l, "bad first argument to ceffects.spawn");

	effects_spawn(p);
	return 0;
}

static int ceffects_win(lua_State* l) {
	checkargs(0, "ceffects.win");
	effects_win();
	return 0;
}

static const luaL_Reg ceffects_fun[] = {
	{"init", ceffects_init},
	{"close", ceffects_close},
	{"update", ceffects_update},
	{"render", ceffects_render},
	{"force_field", ceffects_force_field},
	{"destroy", ceffects_destroy},
	{"spawn", ceffects_spawn},
	{"win", ceffects_win},
	{NULL, NULL}
};

static int csim_init(lua_State* l) {
	checkargs(0, "csim.init");
	sim_init();
	return 0;
}

static int csim_close(lua_State* l) {
	checkargs(0, "csim.close");
	sim_close();
	return 0;
}

static int csim_reset(lua_State* l) {
	checkargs(2, "csim.reset");
	float spawn_interval = luaL_checknumber(l, 1);
	uint start_spawning_at = luaL_checkinteger(l, 2);
	sim_reset(spawn_interval, start_spawning_at);
	return 0;
}

static int csim_spawn(lua_State* l) {
	checkargs(3, "csim.spawn");
	Vector2 p, v;
	if(!_check_vec2(l, 1, &p))
		return luaL_error(l, "bad first argument to csim.spawn");
	if(!_check_vec2(l, 2, &v))
		return luaL_error(l, "bad second argument to csim.spawn");
	uint type = luaL_checkinteger(l, 3);
	float t = time_s();
	sim_spawn(p, v, type, t);
	return 0;
}

static int csim_spawn_random(lua_State* l) {
	checkargs(0, "csim.spawn_random");
	sim_spawn_random();
	return 0;
}

static int csim_force_field(lua_State* l) {
	checkargs(3, "csim.force_field");
	Vector2 p;
	if(!_check_vec2(l, 1, &p))
		return luaL_error(l, "bad first argument to csim.force_field");
	float r = luaL_checknumber(l, 2);
	float strength = luaL_checknumber(l, 3);
	sim_force_field(p, r, strength);
	return 0;
}

static int csim_update(lua_State* l) {
	checkargs(0, "csim.update");
	sim_update();
	return 0;
}

static int csim_render(lua_State* l) {
	checkargs(0, "csim.render");
	sim_render();
	return 0;
}

static int csim_count_alive(lua_State* l) {
	checkargs(0, "csim.count_alive");
	lua_pushinteger(l, sim_count_alive());
	return 1;
}

static int csim_count_ghosts(lua_State* l) {
	checkargs(0, "csim.count_ghosts");
	lua_pushinteger(l, sim_count_ghosts());
	return 1;
}

static int csim_total_mass(lua_State* l) {
	checkargs(0, "csim.total_mass");
	lua_pushinteger(l, sim_total_mass());
	return 1;
}

static int csim_destroy(lua_State* l) {
	checkargs(0, "csim.destroy");
	sim_destroy();
	return 0;
}

static const luaL_Reg csim_fun[] = {
	{"init", csim_init},
	{"close", csim_close},
	{"reset", csim_reset},
	{"spawn", csim_spawn},
	{"spawn_random", csim_spawn_random},
	{"force_field", csim_force_field},
	{"update", csim_update},
	{"render", csim_render},
	{"count_alive", csim_count_alive},
	{"count_ghosts", csim_count_ghosts},
	{"total_mass", csim_total_mass},
	{"destroy", csim_destroy},
	{NULL, NULL}
};

int bind_open_nulis(lua_State* l) {
	luaL_register(l, "ceffects", ceffects_fun);
	luaL_register(l, "csim", csim_fun);
	return 1;
}


