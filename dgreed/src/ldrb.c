#include "ldrb.h"

#include "memory.h"
#include "mml.h"
#include "http.h"

// State
static bool ldrb_ready = false;
static const char* ldrb_server;
static const char* ldrb_gid;
static const char* ldrb_uid;
static LdrbInitCallback ldrb_init_cb = NULL;
static LdrbQueryCallback ldrb_query_cb = NULL;

#define URL_BUFFER_LEN 256
char url_buffer[URL_BUFFER_LEN]; 

static const char* _make_request_url(const char* method) {
	assert(ldrb_server && method);
	assert(strlen(ldrb_server) + strlen(method) < URL_BUFFER_LEN);

	strcpy(url_buffer, ldrb_server);
	strcat(url_buffer, method);

	return url_buffer;
}

static void _ldrb_init_http_cb(HttpResponseCode retcode, const char* data, size_t len, 
		const char* header, size_t header_len) {

	ldrb_ready = retcode == HTTP_OK;
	(*ldrb_init_cb)(ldrb_ready);
}

void ldrb_init(const char* server, const char* gid, const char* uid,
		LdrbInitCallback cb) {
	assert(server && gid && uid);

	// Init state
	ldrb_server = server;
	ldrb_gid = gid;
	ldrb_uid = uid;

	// Do a ping request
	const char* url = _make_request_url("ping");
	http_get(url, false, _ldrb_init_http_cb);
}

void ldrb_close() {

}

static void _ldrb_put_http_cb(HttpResponseCode retcode, const char* data, size_t len,
		const char* header, size_t header_len) {

	if(retcode != HTTP_CREATED)
		LOG_WARNING("ldrb put request failed");
}

#define mml_node_int(mml, name, value) \
	mml_node(mml, #name, ""); \
	mml_setval_int(mml, name, value)

void ldrb_put(const LdrbEntry* entry) {
	if(!ldrb_ready)
		LOG_ERROR("ldrb not ready");

	assert(entry);

	// Convert entry to mml tree
	MMLObject mml;
	mml_empty(&mml);

	NodeIdx root = mml_root(&mml);
	mml_set_name(&mml, root, "entry");

	NodeIdx gid = mml_node(&mml, "gid", ldrb_gid);
	NodeIdx uid = mml_node(&mml, "uid", ldrb_uid);
	NodeIdx flag = mml_node_int(&mml, flag, entry->flag);
	NodeIdx weight = mml_node_int(&mml, weight, entry->weight);

	NodeIdx data = mml_node(&mml, "data", "");
	char* enc_data = base64_encode(entry->data, entry->data_size, NULL); 
	mml_setval_str(&mml, data, enc_data);
	MEM_FREE(enc_data);

	mml_append(&mml, root, gid);
	mml_append(&mml, root, uid);
	mml_append(&mml, root, flag);
	mml_append(&mml, root, weight);
	mml_append(&mml, root, data);

	const char* mml_str = mml_serialize_compact(&mml);

	mml_free(&mml);

	// Do a http post request
	const char* url = _make_request_url("put");
	http_post(url, false, mml_str, NULL, _ldrb_put_http_cb);

	MEM_FREE(mml_str);
}

static void _ldrb_query_http_cb(HttpResponseCode retcode, const char* data, size_t len,
		const char* header, size_t header_len) {
	
	if(retcode == HTTP_CREATED) { // Server returns 201 on success, should be 200
		// Parse response
		MMLObject mml;
		if(!mml_deserialize(&mml, data)) {
			LOG_WARNING("Unable to parse ldrb server response for query");
			(*ldrb_query_cb)(NULL, 0);
			return;
		}

		NodeIdx root = mml_root(&mml);	
		assert(strcmp("entries", mml_get_name(&mml, root)) == 0);
		uint n_entries = mml_count_children(&mml, root);
		assert(n_entries == mml_getval_int(&mml, root));

		// Allocate space for parsed entries
		LdrbEntry* entries = n_entries ? 
			NULL : MEM_ALLOC(n_entries * sizeof(LdrbEntry));

		// Itarate over returned entries, parse each one
		NodeIdx entry = mml_get_first_child(&mml, root);
		for(uint i = 0; entry != 0; entry = mml_get_next(&mml, entry), ++i) {
			assert(strcmp("entry", mml_get_name(&mml, entry)));

			NodeIdx flag = mml_get_child(&mml, entry, "flag");
			assert(flag);
			entries[i].flag = mml_getval_int(&mml, flag);

			NodeIdx weight = mml_get_child(&mml, entry, "weight");
			assert(weight);
			entries[i].weight = mml_getval_int(&mml, weight);

			NodeIdx data = mml_get_child(&mml, entry, "data");
			const char* enc_data = mml_getval_str(&mml, data);
			assert(data);
			uint size;
			entries[i].data = base64_decode(enc_data, strlen(enc_data), &size);
			entries[i].data_size = size;
		}

		// Invoke callback
		(*ldrb_query_cb)(entries, n_entries);

		// Clean up
		for(uint i = 0; i < n_entries; ++i) {
			MEM_FREE(entries[i].data);
		}
		mml_free(&mml);
	}
	else {
		LOG_WARNING("ldrb query request failed");
		(*ldrb_query_cb)(NULL, 0);
	}
}

void lrdb_query(const LdrbQuery* query, LdrbQueryCallback cb) {
	if(!ldrb_ready)
		LOG_ERROR("ldrb not ready!");

	assert(query && cb);

	ldrb_query_cb = cb;

	// Convert query to mml tree
	MMLObject mml;
	mml_empty(&mml);

	NodeIdx root = mml_root(&mml);
	mml_set_name(&mml, root, "query");

	NodeIdx gid = mml_node(&mml, "gid", ldrb_gid);

	NodeIdx flag = mml_node_int(&mml, flag, query->flag);
	NodeIdx startEntry = mml_node_int(&mml, startEntry, query->start_entry);
	NodeIdx entryCount = mml_node_int(&mml, entryCount, query->entry_count);
	NodeIdx startTime = mml_node_int(&mml, startTime, query->start_time);
	NodeIdx endTime = mml_node_int(&mml, endTime, query->end_time);

	mml_append(&mml, root, gid);
	mml_append(&mml, root, flag);
	mml_append(&mml, root, startEntry);
	mml_append(&mml, root, entryCount);
	mml_append(&mml, root, startTime);
	mml_append(&mml, root, endTime);

	const char* mml_str = mml_serialize_compact(&mml);

	mml_free(&mml);

	// Do a http post request
	const char* url = _make_request_url("query");
	http_post(url, false, mml_str, NULL, _ldrb_query_http_cb);

	MEM_FREE(mml_str);
}

