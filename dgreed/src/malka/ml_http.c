#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <http.h>

static int cb_ref = -1;
static lua_State* cb_l = NULL;

static void _ml_http_callback(HttpResponseCode retcode,
		const char* data, size_t len,
		const char* header, size_t header_len) {

	assert(cb_ref != -1);
	assert(cb_l);

	lua_getref(cb_l, cb_ref);
	assert(lua_isfunction(cb_l, -1));

	lua_pushinteger(cb_l, retcode);

	if(data)
		lua_pushlstring(cb_l, data, len);
	else
		lua_pushnil(cb_l);

	if(header)
		lua_pushlstring(cb_l, header, header_len);
	else
		lua_pushnil(cb_l);

	lua_call(cb_l, 3, 0);
	lua_unref(cb_l, cb_ref);
	cb_ref = -1;
}

static int ml_http_get(lua_State* l) {
	int n = lua_gettop(l);

	const char* addr = luaL_checkstring(l, 1);

	if(n == 2) {
		if(!lua_isfunction(l, 2))
			goto error;
		cb_l = l;
		cb_ref = lua_ref(l, 1);
		http_get(addr, false, _ml_http_callback);
		return 0;
	}
	else if(n == 3) {
		bool get_header = lua_toboolean(l, 3);
		lua_pop(l, 1);
		if(!lua_isfunction(l, 2))
			goto error;
		cb_l = l;
		cb_ref = lua_ref(l, 1);
		http_get(addr, get_header, _ml_http_callback);
		return 0;
	}

error:
	return luaL_error(l, "Bad args to http.get");
}

static int ml_http_post(lua_State* l) {
	int n = lua_gettop(l);

	const char* addr = luaL_checkstring(l, 1);
	const char* data = luaL_checkstring(l, 2);
	const char* content_type = NULL;
	bool get_header = false;

	if(n == 4) {
		if(lua_isstring(l, 3))
			content_type = luaL_checkstring(l, 3);
		if(lua_isboolean(l, 4)) {
			get_header = lua_toboolean(l, 4);
			lua_pop(l, 1);
		}
	}
	else if(n == 5) {
		content_type = luaL_checkstring(l, 3);
		get_header = lua_toboolean(l, 5);
		lua_pop(l, 1);
	}
	else if(n != 3)
		goto error;

	if(!lua_isfunction(l, -1))
		goto error;

	cb_l = l;
	cb_ref = lua_ref(l, 1);

	http_post(addr, get_header, data, content_type, _ml_http_callback);

	return 0;

error:
	return luaL_error(l, "Bad args to http.post");
}

static const luaL_Reg http_fun[] = {
	{"get", ml_http_get},
	{"post", ml_http_post},
	{NULL, NULL}
};

int malka_open_http(lua_State* l) {
	luaL_register(l, "http", http_fun);
	lua_pop(l, 1);

	return 1;
}

