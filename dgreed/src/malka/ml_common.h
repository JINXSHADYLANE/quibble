#ifndef ML_COMMON_H
#define ML_COMMON_H

#define checkargs(c, name) \
	int n = lua_gettop(l); \
	if(n != c) \
		return luaL_error(l, "wrong number of arguments provided to " name \
			"; got %d, expected " #c, n)

#endif
