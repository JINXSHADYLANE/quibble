#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <anim.h>

static int ml_anim_init(lua_State* l) {
	checkargs(1, "anim.init");
	const char* filename = luaL_checkstring(l, 1);

	anim_init(filename);
	return 0;
}

static int ml_anim_close(lua_State* l) {
	checkargs(0, "anim.close");

	anim_close();
	return 0;
}

static int ml_anim_new(lua_State* l) {
	checkargs(1, "anim.new");
	const char* name = luaL_checkstring(l, 1);

	Anim* new = anim_new(name);
	lua_pushlightuserdata(l, new);
	return 1;
}

static int ml_anim_del(lua_State* l) {
	checkargs(1, "anim.del");
	assert(lua_islightuserdata(l, 1));

	Anim* anim = lua_touserdata(l, 1);

	anim_del(anim);
	return 0;
}

static int ml_anim_play(lua_State* l) {
	checkargs(2, "anim.play");
	assert(lua_islightuserdata(l, 1));

	Anim* anim = lua_touserdata(l, 1);
	const char* seq = luaL_checkstring(l, 2);

	anim_play(anim, seq);
	return 0;
}

static int ml_anim_frame(lua_State* l) {
	checkargs(1, "anim.frame");
	assert(lua_islightuserdata(l, 1));

	Anim* anim = lua_touserdata(l, 1);

	lua_pushinteger(l, anim_frame(anim));
	return 1;
}

extern int ml_sprsheet_draw_anim_centered(lua_State* l);

static int ml_anim_draw(lua_State* l) {

	// Let's figure out frame number, insert it where it should be
	// and pass the work to sprsheet.draw_anim_centered.
	
	assert(lua_islightuserdata(l, 1));
	Anim* anim = lua_touserdata(l, 1);
	uint frame = anim_frame(anim);
	lua_remove(l, 1);

	lua_pushinteger(l, frame);
	lua_insert(l, 2);

	return ml_sprsheet_draw_anim_centered(l);
}

static const luaL_Reg anim_fun[] = {
	{"init", ml_anim_init},
	{"close", ml_anim_close},
	{"new", ml_anim_new},
	{"del", ml_anim_del},
	{"play", ml_anim_play},
	{"frame", ml_anim_frame},
	{"draw", ml_anim_draw},
	{NULL, NULL}
};

int malka_open_anim(lua_State* l) {
	luaL_register(l, "anim", anim_fun);
	lua_pop(l, 1);
	return 1;
}

