#include "ml_mml.h"
#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include <utils.h>
#include <mml.h>
#include <memory.h>

static void _node_to_table(lua_State* l, MMLObject* obj, NodeIdx node) {
	lua_createtable(l, 0, 3);
	int table = lua_gettop(l);
	lua_pushstring(l, mml_get_name(obj, node));
	lua_pushstring(l, mml_getval_str(obj, node));
	lua_setfield(l, table, "value");
	lua_setfield(l, table, "name");
	
	uint n_children = mml_count_children(obj, node);
	if(n_children > 0) {
		lua_createtable(l, n_children, 0);
		int childs = lua_gettop(l);
		uint i = 1;
		for(NodeIdx child = mml_get_first_child(obj, node); 
			child != 0; 
			child = mml_get_next(obj, child), ++i) {
			_node_to_table(l, obj, child);
			lua_rawseti(l, childs, i);
		}
	}
	else {
		lua_pushnil(l);
	}
	lua_setfield(l, table, "childs");
}

static NodeIdx _table_to_node(lua_State* l, int table, MMLObject* obj, bool root) {
	lua_getfield(l, table, "name");
	lua_getfield(l, table, "value");

	NodeIdx node;
	if(root) {
		node = mml_root(obj);
		mml_set_name(obj, node, lua_tostring(l, -2));
		mml_setval_str(obj, node, lua_tostring(l, -1));
	}		
	else {
		node = mml_node(obj, lua_tostring(l, -2), lua_tostring(l, -1));
	}	
	lua_pop(l, 2);

	lua_getfield(l, table, "childs");
	if(lua_istable(l, -1)) {
		size_t n_children = lua_objlen(l, -1);
		for(uint i = 1; i <= n_children; ++i) {
				lua_rawgeti(l, -1, i);
				int t = lua_gettop(l);
				NodeIdx child = _table_to_node(l, t, obj, false);
				mml_append(obj, node, child);
				lua_pop(l, 1);
		}
	}
	lua_pop(l, 1);

	return node;
}

static int ml_mml_read(lua_State* l) {
	checkargs(1, "mml.read");

	char* text = txtfile_read(luaL_checkstring(l, 1));
	MMLObject obj;
	if(!mml_deserialize(&obj, text))
		return luaL_error(l, "Unable to read mml from file");

	MEM_FREE(text);
	_node_to_table(l, &obj, mml_root(&obj));
	
	mml_free(&obj);

	return 1;
}

static int ml_mml_read_str(lua_State* l) {
	checkargs(1, "mml.read_str");

	MMLObject obj;
	if(!mml_deserialize(&obj, luaL_checkstring(l, 1)))
		return luaL_error(l, "Unable to read mml from string");

	_node_to_table(l, &obj, mml_root(&obj));

	mml_free(&obj);

	return 1;
}

static int ml_mml_write(lua_State* l) {
	checkargs(2, "mml.write");

	MMLObject obj;
	mml_empty(&obj);

	_table_to_node(l, 1, &obj, true);

	char* text = mml_serialize(&obj);
	txtfile_write(luaL_checkstring(l, 2), text);
	MEM_FREE(text);

	mml_free(&obj);

	return 0;
}

static int ml_mml_write_str(lua_State* l) {
	checkargs(1, "mml.write_str");

	MMLObject obj;
	mml_empty(&obj);

	_table_to_node(l, 1, &obj, true);
	char* text = mml_serialize(&obj);
	lua_pushstring(l, text);
	MEM_FREE(text);

	mml_free(&obj);

	return 1;
}

static const luaL_Reg mml_fun[] = {
	{"read", ml_mml_read},
	{"read_str", ml_mml_read_str},
	{"write", ml_mml_write},
	{"write_str", ml_mml_write_str},
	{NULL, NULL}
};

int malka_open_mml(lua_State* l) {
	luaL_register(l, "mml", mml_fun);

	return 1;
}
