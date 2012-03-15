#include "ml_aitvaras.h"

#include <malka/ml_common.h>

#include <malka/lua/lauxlib.h>
#include <malka/lua/lualib.h>

#include <memory.h>
#include <mempool.h>
#include <http.h>
#include <async.h>
#include <datastruct.h>
#include <utils.h>

#include "mongoose.h"

static lua_State* cb_l;

static AATree clients;

static int server_id = 0;

// Mongoose stuff
struct mg_context* mg_ctx;

static const char *standard_reply = "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/plain\r\n"
  "Access-Control-Allow-Origin: *\r\n"
  "Access-Control-Allow-Headers: Content-Type\r\n"
  "Cache: no-cache\r\n"
  "Connection: keep-alive\r\n\r\n";

static void mg_error(struct mg_connection *conn, const struct mg_request_info *ri) {
	mg_printf(conn, "HTTP/1.1 %d XX\r\n" "Connection: close\r\n\r\n", ri->status_code);
	mg_printf(conn, "Error: [%d]", ri->status_code);
}

typedef struct {
	uint id;
	char msg[32];
	const char* method;	
} Invocation;

MemPool invocation_pool;
CriticalSection invocation_cs;

static void _invoke_task(void* userdata) {
	Invocation* inv = (Invocation*)userdata;

	lua_getglobal(cb_l, "aitvaras");
	lua_getfield(cb_l, -1, inv->method);
	lua_pushnumber(cb_l, inv->id);

	if(inv->msg[0] != '\0') {
		lua_call(cb_l, 1, 0);
	}
	else {
		lua_pushstring(cb_l, inv->msg);
		lua_call(cb_l, 2, 0);
	}

	lua_pop(cb_l, 1);

	async_enter_cs(invocation_cs);
	mempool_free(&invocation_pool, inv);	
	async_leave_cs(invocation_cs);
}

static void _invoke(const char* name, uint id, const char* msg) {
	async_enter_cs(invocation_cs);
	Invocation* inv = mempool_alloc(&invocation_pool);	
	async_leave_cs(invocation_cs);

	inv->id = id;
	if(msg) {
		assert(strlen(msg)+1 < 32);
		strcpy(inv->msg, msg);
	}
	else {
		inv->msg[0] = '\0';
	}
	inv->method = name;

	async_schedule(_invoke_task, 0, (void*)inv);
}

static void mg_join(struct mg_connection* conn, const struct mg_request_info* ri) {
	if(strcmp(ri->request_method, "GET") == 0) {
		uint new_id = rand_uint();

		if(!aatree_insert(&clients, new_id, (void*)1))
			goto error;

		mg_printf(conn, "%s", standard_reply);
		mg_printf(conn, "%u\n", new_id);

		// Invoke join_cb
		_invoke("join_cb", new_id, NULL);

		return;
	}
	
error:
	mg_error(conn, ri);
}

static void mg_leave(struct mg_connection* conn, const struct mg_request_info* ri) {
	const char *cl;
	char *buf;
	int len;

	if (strcmp(ri->request_method, "POST") == 0 &&
		(cl = mg_get_header(conn, "Content-Length")) != NULL) {
		len = atoi(cl);
		if ((buf = MEM_ALLOC(len+1)) != NULL) {
			mg_read(conn, buf, len);
			buf[len] = '\0';

			uint id;
			sscanf(buf, "%u", &id); 
			MEM_FREE(buf);

			if(aatree_size(&clients)) {
				aatree_remove(&clients, id);

				// Invoke leave_cb
				_invoke("leave_cb", id, NULL);

				mg_printf(conn, "%s", standard_reply);
				return;
			}
			else {
				goto error;
			}
		}
	}
	else if(strcmp(ri->request_method, "OPTIONS") == 0) {
		mg_printf(conn, "%s", standard_reply);
		return;
	}

error:
	mg_error(conn, ri);
}

static void mg_input(struct mg_connection* conn, const struct mg_request_info* ri) {
	const char *cl;
	char *buf;
	int len;

	mg_printf(conn, "%s", standard_reply);
	if (strcmp(ri->request_method, "POST") == 0 &&
		(cl = mg_get_header(conn, "Content-Length")) != NULL) {
		len = atoi(cl);
		if ((buf = MEM_ALLOC(len+1)) != NULL) {
			mg_read(conn, buf, len);
			buf[len] = '\0';

			uint id;
			char msg[64];

			sscanf(buf, "%u\n:%s", &id, msg);
			MEM_FREE(buf);

			// Invoke input
			_invoke("input_cb", id, msg);
		}
	}
}

static const struct mg_config {
	enum mg_event event;
	const char* uri;
	void (*func)(struct mg_connection*, const struct mg_request_info*);
} mg_config[] = {
	{MG_NEW_REQUEST, "/join", &mg_join},
	{MG_NEW_REQUEST, "/leave", &mg_leave},
	{MG_NEW_REQUEST, "/input", &mg_input},
	{MG_HTTP_ERROR, "", &mg_error},
	{0, NULL, NULL}
};

static void* mg_callback(enum mg_event event, struct mg_connection* conn,
		const struct mg_request_info* request_info) {

	for(int i = 0; mg_config[i].uri != NULL; ++i) {
		if(event == mg_config[i].event &&
				(event == MG_HTTP_ERROR || !strcmp(request_info->uri, mg_config[i].uri))) {
			mg_config[i].func(conn, request_info);
			return "processed";
		}
	}
	return NULL;
}

#define check_conf(name, type) \
	lua_getfield(l, tbl, name); \
	if(!lua_is##type(l, 1)) { \
		LOG_WARNING("No " name); \
		lua_pop(l, 1); \
		return false; \
	} \
	lua_pop(l, 1)

static bool _validate_conf(lua_State* l) {
	int tbl = lua_gettop(l);

	check_conf("lobby_addr", string);
	check_conf("server_addr", string);
	check_conf("listening_port", number);
	check_conf("document_root", string);
	check_conf("join_cb", function);
	check_conf("leave_cb", function);
	check_conf("input_cb", function);

	return true;
}

static const char* _getstr(lua_State* l, const char* name) {
	lua_getfield(l, -1, name);
	const char* str = lua_tostring(l, -1);
	lua_pop(l, 1);
	return str;
}

static void _enlist_cb(HttpResponseCode retcode, const char* data, size_t len,
		const char* header, size_t header_len) {
	if(retcode == 200) {
		sscanf(data, "%d", &server_id);
		LOG_INFO("Enlisted game server, id = %d", server_id);
	}
	else {
		server_id = -1;
		LOG_INFO("Unable to enlist game server");
	}
}

static void _remove_cb(HttpResponseCode retcode, const char* data, size_t len,
		const char* header, size_t header_len) {
	if(retcode == 200) {
		LOG_INFO("Removed game server");
	}
	else {
		LOG_INFO("Unable to remove game server");
	}
}

static int ml_aitvaras_init(lua_State* l) {
	checkargs(0, "aitvaras.init");

	invocation_cs = async_make_cs();
	mempool_init(&invocation_pool, sizeof(Invocation));

	cb_l = l;

	lua_getglobal(l, "aitvaras");	

	if(!_validate_conf(l)) 
		return luaL_error(l, "invalid configuration");

	const char* lobby_addr = _getstr(l, "lobby_addr");
	const char* server_addr = _getstr(l, "server_addr");
	char* enlist_req = alloca(strlen(lobby_addr) + strlen("/enlist") + 1);
	strcpy(enlist_req, lobby_addr);
	strcat(enlist_req, "/enlist");

	http_post(enlist_req, false, server_addr, NULL, _enlist_cb);

	aatree_init(&clients);

	const char* options[] = {
		"listening_ports", _getstr(l, "listening_port"),
		"document_root", _getstr(l, "document_root"),
		NULL
	};
	mg_ctx = mg_start(mg_callback, NULL, options);

	lua_pop(l, 1);

	return 0;
}

static void _cleanup_invocations(void* userdata) {
	mempool_drain(&invocation_pool);
}

static int ml_aitvaras_close(lua_State* l) {
	checkargs(0, "aitvaras.close");

	lua_getglobal(l, "aitvaras");

	const char* lobby_addr = _getstr(l, "lobby_addr");
	const char* server_addr = _getstr(l, "server_addr");
	char* remove_req = alloca(strlen(lobby_addr) + strlen("/remove") + 1);
	strcpy(remove_req, lobby_addr);
	strcat(remove_req, "/remove");

	http_post(remove_req, false, server_addr, NULL, _remove_cb);

	mg_stop(mg_ctx);

	aatree_free(&clients);

	// Since some invocations might still be live, append
	// cleanup task to the end of the queue
	async_schedule(_cleanup_invocations, 0, NULL); 

	return 0;
}

static int ml_aitvaras_id(lua_State* l) {
	checkargs(0, "aitvaras.id");

	lua_pushnumber(l, server_id);
	
	return 1;
}

static const luaL_Reg aitvaras_fun[] = {
	{"init", ml_aitvaras_init},
	{"close", ml_aitvaras_close},
	{"id", ml_aitvaras_id},
	{NULL, NULL}
};

int malka_open_aitvaras(lua_State* l) {
	luaL_register(l, "aitvaras", aitvaras_fun);
	lua_pop(l, 1);
	return 1;
}

