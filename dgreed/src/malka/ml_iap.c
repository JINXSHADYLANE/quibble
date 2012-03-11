#include "ml_iap.h"
#include "ml_common.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#ifdef TARGET_IOS

#include "iap.h"

static void _product_to_tbl(lua_State* l, Product* p) {
	lua_createtable(l, 0, 4);
	int tbl = lua_gettop(l);

	lua_pushstring(l, "id");
	lua_pushstring(l, p->id);
	lua_rawset(l, tbl);

	lua_pushstring(l, "title");
	lua_pushstring(l, p->title);
	lua_rawset(l, tbl);

	lua_pushstring(l, "description");
	lua_pushstring(l, p->description);
	lua_rawset(l, tbl);

	lua_pushstring(l, "price_str");
	lua_pushstring(l, p->price_str);
	lua_rawset(l, tbl);
}

static lua_State* cb_l = NULL;
static int cb_purchase_ref = -1;
static int cb_product_ref = -1;

static void _products_callback(DArray* products) {
	assert(cb_l);
	if(products) {
		Product* prs = DARRAY_DATA_PTR(*products, Product);
		lua_createtable(cb_l, products->size, 0);
		int tbl = lua_gettop(cb_l);
		for(uint i = 0; i < products->size; ++i) {
			_product_to_tbl(cb_l, &prs[i]);
			lua_rawseti(cb_l, tbl, i+1);
		}

		lua_getref(cb_l, cb_product_ref);
		lua_insert(cb_l, -2);
		lua_call(cb_l, 1, 0);
	}
}

static void _purchase_callback(const char* id, bool success) {
	assert(cb_l);
	lua_getref(cb_l, cb_purchase_ref);
	lua_pushstring(cb_l, id);
	lua_pushboolean(cb_l, success);
	lua_call(cb_l, 2, 0);	
}

static int ml_iap_init(lua_State* l) {
	checkargs(2, "iap.init");

	cb_l = l;
	cb_purchase_ref = lua_ref(l, 1);
	cb_product_ref = lua_ref(l, 1);

	iap_init(_products_callback, _purchase_callback);

	return 0;
}

static int ml_iap_close(lua_State* l) {
	checkargs(0, "iap.close");

	lua_unref(l, cb_product_ref);
	lua_unref(l, cb_purchase_ref);

	iap_close();

	return 0;
}

static int ml_iap_is_active(lua_State* l) {
	checkargs(0, "iap.is_active");

	lua_pushboolean(l, iap_is_active());

	return 1;
}

static int ml_iap_purchase(lua_State* l) {
	checkargs(1, "iap.purchase");

	const char* id = luaL_checkstring(l, 1);

	iap_purchase(id);

	return 0;
}

static const luaL_Reg iap_fun[] = {
	{"init", ml_iap_init},
	{"close", ml_iap_close},
	{"is_active", ml_iap_is_active},
	{"purchase", ml_iap_purchase},
	{NULL, NULL}
};

int malka_open_iap(lua_State* l) {
	luaL_register(l, "iap", iap_fun);
	lua_pop(l, 1);

	return 1;
}

#else

int malka_open_iap(lua_State* l) {
	lua_pushnil(l);
	lua_setglobal(l, "iap");

	return 1;
}

#endif

