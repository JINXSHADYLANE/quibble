#include <system.h>
#include <font.h>
#include <memory.h>
#include <http.h>
#include <datastruct.h>
#include <sprsheet.h>

#include "mongoose.h"

#define WIDTH 1024
#define HEIGHT 768
#define MAX_CLIENTS 32
#define ASSETS_PRE "aitvaras_assets/"
#define SERVER_ADDR "http://89.249.93.114:8008"
#define LOBBY_ADDR "http://89.249.93.114:80"
#define LISTENING_PORT "8008"

FontHandle font;
const char* id_text = NULL;

typedef struct {
	Vector2 pos;
	Vector2 dir;
} Client;

AATree clients;
uint n_ids = 0;
uint ids[MAX_CLIENTS];

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

static void mg_join(struct mg_connection* conn, const struct mg_request_info* ri) {
	if(strcmp(ri->request_method, "GET") == 0) {
		uint new_id = rand_uint();

		Client* new = MEM_ALLOC(sizeof(Client));
		if(!aatree_insert(&clients, new_id, new))
			goto error;

		assert(n_ids < MAX_CLIENTS);
		ids[n_ids++] = new_id;

		new->pos = vec2(
			rand_float_range(0, WIDTH),
			rand_float_range(0, HEIGHT)
		);
		new->dir = vec2(0.0f, 0.0f);

		mg_printf(conn, "%s", standard_reply);
		mg_printf(conn, "%u\n", new_id);
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
				Client* c = aatree_remove(&clients, id);
				if(!c)
					goto error;

				for(uint i = 0; i < n_ids; ++i) {
					if(ids[i] == id) {
						ids[i] = ids[--n_ids];
						goto ok;
					}
				}
				goto error;
					
			ok:
				mg_printf(conn, "%s", standard_reply);
				MEM_FREE(c);
				return;
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
			char dir;
			char state;

			sscanf(buf, "%u\n:%c:%c", &id, &dir, &state);
			MEM_FREE(buf);

			Client* c = aatree_find(&clients, id);
			if(dir == 'l') 
				c->dir = vec2(state == 'd' ? -1.0f : 0.0f, c->dir.y);
			if(dir == 'r')
				c->dir = vec2(state == 'd' ? 1.0f : 0.0f, c->dir.y);
			if(dir == 'u')
				c->dir = vec2(c->dir.x, state == 'd' ? -1.0f : 0.0f);
			if(dir == 'd')
				c->dir = vec2(c->dir.x, state == 'd' ? 1.0f : 0.0f);
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

void enlist_cb(HttpResponseCode retcode, const char* data, size_t len,
		const char* header, size_t header_len) {
	if(retcode == 200) {
		id_text = strclone(data);
	}
	else {
		id_text = strclone("unable to reach lobby server");
	}
}

void remove_cb(HttpResponseCode retcode, const char* data, size_t len, 
		const char* header, size_t header_len) {
	if(retcode == 200) {
		printf("Removed!\n");
	}
	else {
		printf("remove failed\n");
	}
}

bool dgreed_init(void) {
	rand_init(413);

	video_init(1024, 768, "Aitvaras");
	sound_init();

	http_post(LOBBY_ADDR "/enlist", false, SERVER_ADDR, NULL, enlist_cb);

	font = font_load(ASSETS_PRE "lucida_grande_60px.bft");

	aatree_init(&clients);
	sprsheet_init(ASSETS_PRE "sprsheet.mml");

  	const char *options[] = {
		"listening_ports", LISTENING_PORT, 
		"document_root", "aitvaras_html/",
		NULL
	};
	mg_ctx = mg_start(mg_callback, NULL, options);

	return true;
}

bool dgreed_update(void) {
	sound_update();

	if(key_up(KEY_QUIT)) {
		http_post(LOBBY_ADDR "/remove", false, SERVER_ADDR, NULL, remove_cb);
		return false;
	}

	return true;
}

bool dgreed_render(void) {
	
	if(id_text) {
		Vector2 pos = {10.0, 680.0f};
		font_draw(font, id_text, 6, &pos, COLOR_WHITE);
	}

	for(uint i = 0; i < n_ids; ++i) {
		uint id = ids[i];
		Client* c = aatree_find(&clients, id);

		c->pos = vec2_add(c->pos, vec2_scale(c->dir, 0.8f));

		spr_draw_cntr("white", 1, c->pos, 0.0f, 1.0f, COLOR_WHITE);	
	}
	
	video_present();

	return true;
}

void dgreed_close(void) {
	mg_stop(mg_ctx);

	sprsheet_close();

	while(aatree_size(&clients)) {
		Client* c;
		uint id = aatree_min(&clients, (void**)&c);
		aatree_remove(&clients, id);
		MEM_FREE(c);
	}

	aatree_free(&clients);

	if(id_text)
		MEM_FREE(id_text);

	font_free(font);

	sound_close();
	video_close();
}

#ifndef TARGET_IOS
int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);
	
	dgreed_init();
	while(system_update()) {
		if(!dgreed_update())
			break;
		if(!dgreed_render())
			break;
	}	
	dgreed_close();

	return 0;
}	
#endif
