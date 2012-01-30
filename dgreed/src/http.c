#include "http.h"

#include "async.h"

#include <curl/curl.h>

#define INITIAL_HEADER_BUF 1024
#define INITIAL_DATA_BUF 2048

typedef struct {
	char* data;
	char* cursor_ptr;
	size_t size;
} MemBuffer;

typedef struct {
	const char* data;
	const char* cursor_ptr;
	size_t size;
} ReadBuffer;

// Shared data
static bool http_initialized = false;
static bool shr_live_req = false;
static const char* shr_req_addr;
static bool shr_req_get_header;
static const char* shr_req_data;
static const char* shr_req_content_type;
static HttpCallback shr_req_cb;

static CriticalSection shr_data_cs;
static MemBuffer shr_header;
static MemBuffer shr_data;
static long shr_resp_code;

// io thread data
static CURL* io_curl;
static MemBuffer io_header;
static MemBuffer io_data;
static ReadBuffer io_readbuf;
static long io_resp_code;

static void _http_invoke_callback(void* userdata);

// Stuff happening on io thread

static size_t _io_write_data(char* ptr, size_t size, size_t nmemb, void* userdata) {
	assert(userdata);
	MemBuffer* buffer = userdata;

	size_t data_size = size * nmemb;

	if(buffer->cursor_ptr + data_size >= buffer->data + buffer->size) {
		// Alloc more space
		char* old_data = buffer->data;
		buffer->size *= 2;
		buffer->data = realloc(buffer->data, buffer->size);
		buffer->cursor_ptr = buffer->data + (buffer->cursor_ptr - old_data); 
	}

	assert(buffer->cursor_ptr + data_size < buffer->data + buffer->size);

	memcpy(buffer->cursor_ptr, ptr, data_size);
	buffer->cursor_ptr += data_size;

	return data_size;
}

static size_t _io_read_data(char* ptr, size_t size, size_t nmemb, void* userdata) {
	assert(userdata);
	ReadBuffer* buffer = userdata;

	size_t data_size = size * nmemb;
	size_t can_read = buffer->size - (buffer->cursor_ptr - buffer->data);
	
	if(can_read) {
		size_t bytes_read = MIN(can_read, data_size);
		memcpy(ptr, buffer->data, bytes_read);
		buffer->cursor_ptr += bytes_read;
		return bytes_read;
	}

	return 0;
}

static void _io_http_init(void* userdata) {
	assert(!http_initialized);

	io_header.data = malloc(INITIAL_HEADER_BUF);
	io_header.size = INITIAL_HEADER_BUF; 
	io_data.data = malloc(INITIAL_DATA_BUF);
	io_data.size = INITIAL_DATA_BUF;

	shr_header.data = malloc(INITIAL_HEADER_BUF);
	shr_header.size = INITIAL_HEADER_BUF; 
	shr_header.cursor_ptr = NULL;
	shr_data.data = malloc(INITIAL_DATA_BUF);
	shr_data.size = INITIAL_DATA_BUF;
	shr_data.cursor_ptr = NULL;

	shr_data_cs = async_make_cs();

	io_curl = curl_easy_init();	

	http_initialized = true;
	LOG_INFO("http initialized - %s", curl_version());
}

static void _io_http_close(void* userdata) {
	assert(http_initialized);

	curl_easy_cleanup(io_curl);
}

static void _io_move_membuf(MemBuffer* dest, MemBuffer* src) {
	if(src->cursor_ptr > src->data) {
		size_t real_size = src->cursor_ptr - src->data;
		if(dest->size < real_size) {
			dest->size = real_size;
			// We don't need to preserve data, so don't do realloc
			free(dest->data);
			dest->data = malloc(real_size);
		}
		assert(dest->size >= real_size);
		memcpy(dest->data, src->data, real_size);
		dest->cursor_ptr = dest->data + real_size;
	}
}

static void _io_move_data() {
	async_enter_cs(shr_data_cs);

	shr_resp_code = io_resp_code;
	_io_move_membuf(&shr_header, &io_header);
	_io_move_membuf(&shr_data, &io_data);

	async_leave_cs(shr_data_cs);
}

static void _io_http_get(void* userdata) {
	assert(http_initialized && shr_live_req);
	
	// Reset buffer write pointers
	io_header.cursor_ptr = io_header.data;
	io_data.cursor_ptr = io_data.data;

	// Set up curl request
	curl_easy_setopt(io_curl, CURLOPT_HTTPGET, 1);
	curl_easy_setopt(io_curl, CURLOPT_URL, shr_req_addr);
	curl_easy_setopt(io_curl, CURLOPT_HEADER, shr_req_get_header ? 1 : 0);
	curl_easy_setopt(io_curl, CURLOPT_WRITEDATA, &io_data);
	curl_easy_setopt(io_curl, CURLOPT_WRITEFUNCTION, _io_write_data);
	if(shr_req_get_header) {
		curl_easy_setopt(io_curl, CURLOPT_WRITEHEADER, &io_header);
		curl_easy_setopt(io_curl, CURLOPT_HEADERFUNCTION, _io_write_data);
	}
	else {
		curl_easy_setopt(io_curl, CURLOPT_WRITEHEADER, 0);
	}

	// Perform request
	CURLcode res = curl_easy_perform(io_curl);
	if(res != 0) {
		LOG_WARNING("http get request to %s did not complete", shr_req_addr);
		io_resp_code = -1;
	}
	else {
		curl_easy_getinfo(io_curl, CURLINFO_RESPONSE_CODE, &io_resp_code);
	}

	_io_move_data();

	// Schedule callback to be executed on the main thread
	async_schedule(_http_invoke_callback, 0, shr_req_cb);
}

static void _io_http_post(void* userdata) {
	assert(http_initialized && shr_live_req);

	// Reset buffer write pointers
	io_header.cursor_ptr = io_header.data;
	io_data.cursor_ptr = io_data.data;

	// Set up read buffer
	io_readbuf.data = shr_req_data;
	io_readbuf.cursor_ptr = io_readbuf.data;
	io_readbuf.size = shr_req_data ? strlen(shr_req_data) : 0;

	// Set up curl request
	curl_easy_setopt(io_curl, CURLOPT_POST, 1);
	curl_easy_setopt(io_curl, CURLOPT_URL, shr_req_addr);
	curl_easy_setopt(io_curl, CURLOPT_HEADER, shr_req_get_header ? 1 : 0);
	curl_easy_setopt(io_curl, CURLOPT_WRITEDATA, &io_data);
	curl_easy_setopt(io_curl, CURLOPT_WRITEFUNCTION, _io_write_data);
	if(shr_req_get_header) {
		curl_easy_setopt(io_curl, CURLOPT_WRITEHEADER, &io_header);
		curl_easy_setopt(io_curl, CURLOPT_HEADERFUNCTION, _io_write_data);
	}
	else {
		curl_easy_setopt(io_curl, CURLOPT_WRITEHEADER, 0);
	}
	curl_easy_setopt(io_curl, CURLOPT_READDATA, &io_readbuf);
	curl_easy_setopt(io_curl, CURLOPT_READFUNCTION, _io_read_data);
	curl_easy_setopt(io_curl, CURLOPT_POSTFIELDSIZE, io_readbuf.size);

	// Set custom content type header, if neccessary
	struct curl_slist* chunk = NULL;
	if(shr_req_content_type) {
		char content_type[128] = "Content-Type: ";
		assert(strlen(content_type) + strlen(shr_req_content_type) < 128);
		strcat(content_type, shr_req_content_type);
		chunk = curl_slist_append(chunk, content_type); 
		curl_easy_setopt(io_curl, CURLOPT_HTTPHEADER, chunk);
	}

	// Perform request
	CURLcode res = curl_easy_perform(io_curl);
	if(res != 0) {
		LOG_WARNING("http post request to %s did not complete", shr_req_addr);
		io_resp_code = -1;
	}
	else {
		curl_easy_getinfo(io_curl, CURLINFO_RESPONSE_CODE, &io_resp_code);
	}

	if(chunk)
		curl_slist_free_all(chunk);

	_io_move_data();

	// Schedule callback to be executed on the main thread
	async_schedule(_http_invoke_callback, 0, shr_req_cb);
}

// Stuff happening on main thread

static void _http_invoke_callback(void* userdata) {
	assert(userdata);
	assert(shr_live_req);
	
	HttpCallback cb = userdata;

	async_enter_cs(shr_data_cs);

	size_t data_size = shr_data.cursor_ptr - shr_data.data;
	size_t header_size = 0;
	if(shr_req_get_header) {
		header_size = shr_header.cursor_ptr - shr_header.data;
	}

	// Callback
	(*cb)(
			shr_resp_code, 
			shr_data.data, data_size, 
			header_size ? shr_header.data : NULL, header_size
	);

	async_leave_cs(shr_data_cs);

	shr_live_req = false;
}

static void _http_init(void) {
	assert(!http_initialized); 

	curl_global_init(CURL_GLOBAL_ALL);

	async_run_io(_io_http_init, NULL);
}

static void _http_close(void) {
	assert(http_initialized);

	TaskId task = async_run_io(_io_http_close, NULL);

	// Wait for task to complete
	while(!async_is_finished(task)) {
	};

	curl_global_cleanup();

	http_initialized = false;

	LOG_INFO("http closed");
}

static void _http_check_init(void) {
	if(!http_initialized)
		_http_init();
}

void _http_check_close(void) {
	if(http_initialized)
		_http_close();
}

void http_get(const char* addr, bool get_header, HttpCallback cb) {
	assert(addr && cb);
	assert(!shr_live_req);
	_http_check_init();

	// Pass data to io task
	shr_live_req = true;
	shr_req_addr = addr;
	shr_req_get_header = get_header;
	shr_req_cb = cb;

	async_run_io(_io_http_get, NULL);
}

void http_post(
		const char* addr, bool get_header,
		const char* data, const char* content_type, HttpCallback cb) {
	assert(addr && cb);
	assert(!shr_live_req);

	// Pass data to io task
	shr_live_req = true;
	shr_req_addr = addr;
	shr_req_get_header = get_header;
	shr_req_data = data;
	shr_req_content_type = content_type;
	shr_req_cb = cb;

	async_run_io(_io_http_post, NULL);
}

