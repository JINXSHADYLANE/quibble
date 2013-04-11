#ifndef ML_COMMON_H
#define ML_COMMON_H

#ifdef _DEBUG
#define checkargs(c, name) \
	int n = lua_gettop(l); \
	if(n != c) \
		return luaL_error(l, "wrong number of arguments provided to " name \
			"; got %d, expected " #c, n)
#else
#define checkargs(c, name) (void)c;(void)name
#endif

#define malka_register(l, name, fun) luaL_register(l, name, fun); lua_pop(l, 1)

#endif
