#ifndef HTTP_H
#define HTTP_H

#include <utils.h>

typedef enum {
	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_ACCEPTED = 202,
	HTTP_NO_CONTENT = 204,
	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	HTTP_METHOD_NOT_ALLOWED = 405,
	HTTP_IM_A_TEAPOT = 418,
	HTTP_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501
} HttpResponseCode;

typedef void (*HttpCallback)(
		HttpResponseCode retcode,
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
