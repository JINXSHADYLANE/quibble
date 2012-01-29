#ifndef HTTP_H
#define HTTP_H

#include <utils.h>

typedef void (*HttpCallback)(
		int retcode,
		const char* data, size_t len,
		const char* header, size_t header_len
);

void http_get(const char* addr, bool get_header, HttpCallback cb);

void http_post(
	const char* addr, bool get_header,
	const char* data, const char* content_type, HttpCallback cb);

void http_put(const char* addr, void* data, size_t size, 
	bool get_header, HttpCallback cb);

void http_delete(const char* addr, bool get_header, HttpCallback cb);

#endif
