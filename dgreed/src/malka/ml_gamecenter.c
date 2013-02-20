#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#ifdef TARGET_IOS

#include "gamecenter.h"

static int ml_gamecenter_init(lua_State* l) {
	checkargs(0, "gamecenter.init");

	gamecenter_init();

	return 0;
}

static int ml_gamecenter_close(lua_State* l) {
	checkargs(0, "gamecenter.close");

	gamecenter_close();

	return 0;
}

static int ml_gamecenter_is_active(lua_State* l) {
	checkargs(0, "gamecenter.is_active");
	
	lua_pushboolean(l, gamecenter_is_active());

	return 1;
}

static int ml_gamecenter_report_score(lua_State* l) {
	checkargs(3, "gamecenter.report_score");

	const char* category = luaL_checkstring(l, 1);
	int context = luaL_checkinteger(l, 2);
	int value = luaL_checkinteger(l, 3);

	GameCenterScore score = {
		.category = category,
		.context = (int64)context,
		.value = (int64)value,
		.player = NULL
	};

	gamecenter_report_score(&score);
	
	return 0;
}

static PlayerScope _str_to_player_scope(const char* ps) {
	if(strcmp(ps, "everyone") == 0)
		return PS_GLOBAL;
	if(strcmp(ps, "friends") == 0)
		return PS_FRIENDS;
	assert(0 && "Bad player scope string");
	return PS_GLOBAL;
}

static TimeScope _str_to_time_scope(const char* ts) {
	if(strcmp(ts, "day") == 0)
		return TS_DAY;
	if(strcmp(ts, "week") == 0)
		return TS_WEEK;
	if(strcmp(ts, "all") == 0)
		return TS_ALL;
	assert(0 && "Bad time scope string");
	return TS_ALL;
}

static lua_State* cb_l = NULL;
static int cb_score_ref = -1;
static bool live_scores_req = false;

static void _score_to_tbl(lua_State* l, GameCenterScore* s) {
	lua_createtable(l, 0, 4);
	int tbl = lua_gettop(l);

	lua_pushstring(l, "category");
	lua_pushstring(l, s->category);
	lua_rawset(l, tbl);

	lua_pushstring(l, "context");
	lua_pushinteger(l, (int)s->context);
	lua_rawset(l, tbl);

	lua_pushstring(l, "value");
	lua_pushinteger(l, (int)s->value);
	lua_rawset(l, tbl);

	lua_pushstring(l, "player");
	lua_pushstring(l, s->player);
	lua_rawset(l, tbl);
}

static void _get_scores_cb(DArray* data) {
	assert(live_scores_req);
	if(data) {
		GameCenterScore* scores = DARRAY_DATA_PTR(*data, GameCenterScore);
		lua_createtable(cb_l, data->size, 0);
		int tbl = lua_gettop(cb_l);

		for(uint i = 0; i < data->size; ++i) {
			GameCenterScore* s = &scores[i];
			_score_to_tbl(cb_l, s);
			lua_rawseti(cb_l, tbl, i+1);
		}
	}
	else {
		// Call with empty table
		lua_createtable(cb_l, 0, 0);
	}

	lua_getref(cb_l, cb_score_ref);
	assert(lua_isfunction(cb_l, -1));
	lua_call(cb_l, 1, 0);	
	lua_unref(cb_l, cb_score_ref);

	live_scores_req = false;
}

static int ml_gamecenter_get_scores(lua_State* l) {
	checkargs(5, "gamecenter.get_scores");

	if(live_scores_req)
		return luaL_error(l, "there is unfinished scores request in the background");
	live_scores_req = true;

	int start = luaL_checkinteger(l, 1);
	int len = luaL_checkinteger(l, 2);
	const char* timescope = luaL_checkstring(l, 3);
	const char* playerscope = luaL_checkstring(l, 4);

	if(lua_isfunction(l, 5)) {
		cb_score_ref = lua_ref(l, 1); 
		cb_l = l;

		PlayerScope ps = _str_to_player_scope(playerscope);
		TimeScope ts = _str_to_time_scope(timescope);

		gamecenter_get_scores(start, len, ts, ps, _get_scores_cb);			

		return 0;
	}
	else {
		return luaL_error(l, "last gamecenter.get_scores arg must be callback");
	}
}

static int ml_gamecenter_show_leaderboard(lua_State* l) {
	checkargs(2, "gamecenter.show_leaderboard");

	const char* category = luaL_checkstring(l, 1);
	const char* timescope = luaL_checkstring(l, 2);
	TimeScope ts = _str_to_time_scope(timescope);

	gamecenter_show_leaderboard(category, ts);

	return 0;
}

static int ml_gamecenter_report_achievement(lua_State* l) {
	checkargs(2, "gamecenter.report_achievement");

	const char* id = luaL_checkstring(l, 1);
	float percent = luaL_checknumber(l, 2);

	gamecenter_report_achievement(id, percent);

	return 0;
}

int cb_achievement_ref = -1;
bool live_achievements_req = false;

static void _achievement_to_tbl(lua_State* l, GameCenterAchievement* a) {
	lua_createtable(l, 0, 5);
	int tbl = lua_gettop(l);

	lua_pushstring(l, "id");
	lua_pushstring(l, a->identifier);
	lua_rawset(l, tbl);

	lua_pushstring(l, "desc");
	lua_pushstring(l, a->description);
	lua_rawset(l, tbl);

	lua_pushstring(l, "title");
	lua_pushstring(l, a->title);
	lua_rawset(l, tbl);

	lua_pushstring(l, "progress");
	lua_pushnumber(l, a->percent);
	lua_rawset(l, tbl);

	lua_pushstring(l, "completed");
	lua_pushboolean(l, a->completed);
	lua_rawset(l, tbl);
}

static void _get_achievements_cb(DArray* data) {
	assert(live_achievements_req);
    
    lua_getref(cb_l, cb_achievement_ref);
	assert(lua_isfunction(cb_l, -1));

	if(data) {
		GameCenterAchievement* achievements = DARRAY_DATA_PTR(*data, GameCenterAchievement);
		lua_createtable(cb_l, data->size, 0);
		int tbl = lua_gettop(cb_l);

		for(uint i = 0; i < data->size; ++i) {
			GameCenterAchievement* a = &achievements[i];
			_achievement_to_tbl(cb_l, a);
			lua_rawseti(cb_l, tbl, i+1);
		}
	}
	else {
		lua_createtable(cb_l, 0, 0);
	}

	lua_call(cb_l, 1, 0);
	lua_unref(cb_l, cb_achievement_ref);

	live_achievements_req = false;
}

static int ml_gamecenter_get_achievements(lua_State* l) {
	checkargs(1, "gamecenter.get_achievements");

	if(live_achievements_req)
		return luaL_error(l, "there is unfinished achievement request in the background");
	live_achievements_req = true;

	if(lua_isfunction(l, 1)) {
		cb_achievement_ref = lua_ref(l, 1);
		cb_l = l;

		gamecenter_get_achievements(_get_achievements_cb);

		return 0;
	}
	else {
		return luaL_error(l, "gamecenter.get_achivements arg must be callback");
	}
}

static int ml_gamecenter_show_achievements(lua_State* l) {
	checkargs(0, "gamecenter.show_achievements");

	gamecenter_show_achievements();

	return 0;
}

static const luaL_Reg gamecenter_fun[] = {
	{"init", ml_gamecenter_init},
	{"close", ml_gamecenter_close},
	{"is_active", ml_gamecenter_is_active},
	{"report_score", ml_gamecenter_report_score},
	{"get_scores", ml_gamecenter_get_scores},
	{"show_leaderboard", ml_gamecenter_show_leaderboard},
	{"report_achievement", ml_gamecenter_report_achievement},
	{"get_achievements", ml_gamecenter_get_achievements},
	{"show_achievements", ml_gamecenter_show_achievements},
	{NULL, NULL}
};

int malka_open_gamecenter(lua_State* l) {
	malka_register(l, "gamecenter", gamecenter_fun);
	return 1;
}

#else

int malka_open_gamecenter(lua_State* l) {
	lua_pushnil(l);
	lua_setglobal(l, "gamecenter");

	return 1;
}

#endif
