#include "ml_os.h"
#include "ml_common.h"

#include <utils.h>

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#ifdef TARGET_IOS

extern void ios_open_web_url(const char* url);
extern void ios_alert(const char* title, const char* text);
extern bool ios_has_twitter(void);
extern void ios_tweet(const char* msg, const char* addr);

typedef void (*MailCallback)(const char* result);

extern bool ios_has_mail(void);
extern void ios_mail(const char* subject, const char* body, MailCallback cb);

static int ml_os_open(lua_State* l) {
	checkargs(1, "os.open");

	const char* url = luaL_checkstring(l, 1);
	ios_open_web_url(url);
	
	return 0;
}

static int ml_os_alert(lua_State* l) {
	checkargs(2, "os.alert");

	const char* title = luaL_checkstring(l, 1);
	const char* text = luaL_checkstring(l, 2);

	ios_alert(title, text);

	return 0;
}

static int ml_os_has_twitter(lua_State* l) {
	checkargs(0, "os.has_twitter");

	lua_pushboolean(l, ios_has_twitter());

	return 1;
}

static int ml_os_tweet(lua_State* l) {
	int n = lua_gettop(l);

	const char* msg = luaL_checkstring(l, 1);
	const char* url = NULL;

	if(n == 2) {
		url = luaL_checkstring(l, 2);
	}

	ios_tweet(msg, url);

	return 0;
}

static int ml_os_has_mail(lua_State* l) {
	checkargs(0, "os.has_mail");

	lua_pushboolean(l, ios_has_mail());
	
	return 1;
}

static lua_State* cb_l = NULL;
static int cb_mail_ref = -1;

static void _mail_cb(const char* result) {
	assert(result && cb_l && cb_mail_ref != -1);
	
	lua_getref(cb_l, cb_mail_ref);
	assert(lua_isfunction(cb_l, -1));

	lua_pushstring(cb_l, result);

	lua_call(cb_l, 1, 0);
	lua_unref(cb_l, cb_mail_ref);
	cb_mail_ref = -1;
}

static int ml_os_mail(lua_State* l) {
	checkargs(3, "os.mail");

	const char* subject = luaL_checkstring(l, 1);
	const char* body = luaL_checkstring(l, 2);

	cb_l = l;
	cb_mail_ref = lua_ref(l, 1);

	ios_mail(subject, body, _mail_cb);

	return 0;
}

#else

static int ml_os_open(lua_State* l) {
	checkargs(1, "os.open");

	return 0;
}

static int ml_os_alert(lua_State* l) {
	checkargs(2, "os.alert");

	return 0;
}

static int ml_os_has_twitter(lua_State* l) {
	lua_pushboolean(l, false);

	return 1;
}

static int ml_os_tweet(lua_State* l) {
	return 0;
}

static int ml_os_has_mail(lua_State* l) {
	lua_pushboolean(l, false);

	return 1;
}

static int ml_os_mail(lua_State* l) {
	return 0;
}

#endif

static int ml_os_move(lua_State* l) {
	checkargs(2, "os.move");
	const char* old = luaL_checkstring(l, 1);
	const char* new = luaL_checkstring(l, 2);
	file_move(old, new);
	return 0;
}

static int ml_os_remove(lua_State* l) {
	checkargs(1, "os.remove");
	const char* name = luaL_checkstring(l, 1);
	file_remove(name);
	return 0;
}

static luaL_Reg os_fun[] = {
	{"open", ml_os_open},
	{"alert", ml_os_alert},
	{"has_twitter", ml_os_has_twitter},
	{"tweet", ml_os_tweet},
	{"move", ml_os_move},
	("remove", ml_os_remove},
	{"has_mail", ml_os_has_mail},
	{"mail", ml_os_mail},
	{NULL, NULL}
};

int malka_open_os(lua_State* l) {
	luaL_register(l, "os", os_fun);

	lua_pop(l, 1);

	return 1;
}

