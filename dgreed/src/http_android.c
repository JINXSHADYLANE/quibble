#include "http.h"

#include <SDL.h>
#include <jni.h>
#include "async.h"

#define max_callbacks 16

typedef struct {
	int id;
	HttpCallback cb;
} CallbackRecord;

static bool http_initialized = false;
static CriticalSection http_cs;
static int id_counter = 1;
static uint n_callbacks = 0;
static CallbackRecord callbacks[max_callbacks] = {{0, NULL}};

static void _http_check_init(void) {
	if(!http_initialized) {
		http_initialized = true;
		http_cs = async_make_cs();
	}
}

static int _register_cb(HttpCallback cb) {
	async_enter_cs(http_cs);

	CallbackRecord new = {
		.id = id_counter++,
		.cb = cb 
	};

	assert(n_callbacks < max_callbacks);
	callbacks[n_callbacks++] = new;

	async_leave_cs(http_cs);

	return new.id;
}

static void _invoke_cb(int id, uint retcode, const char* data, const char* header) {
	async_enter_cs(http_cs);

	assert(n_callbacks);

	// Find the right record
	CallbackRecord* rec = NULL;
	for(uint i = 0; i < n_callbacks; ++i) {
		if(callbacks[i].id == id) {
			rec = &callbacks[i];
			break;
		}
	}

	if(!rec)
		LOG_ERROR("Error finding the right http callback");

	size_t data_len = data ? strlen(data) : 0;
	size_t header_len = header ? strlen(header) : 0;

	// Invoke the callback
	rec->cb(retcode, data, data_len, header, header_len); 	

	// Remove record by overwriting it with the last one and decreasing count
	*rec = callbacks[--n_callbacks];

	async_leave_cs(http_cs);
}

void Java_com_quibble_dgreed_Http_nativeCallback(
	JNIEnv* env, jclass cls, jint callbackId, 
	jint retcode, jstring data, jstring header
) {

	const char* data_cstr = NULL;
	const char* header_cstr = NULL;
	if(data)
		data_cstr = (*env)->GetStringUTFChars(env, data, 0); 
	if(header)
		header_cstr = (*env)->GetStringUTFChars(env, header, 0);

	_invoke_cb(callbackId, retcode, data_cstr, header_cstr);

	if(data_cstr)
		(*env)->ReleaseStringUTFChars(env, data, data_cstr);
	if(header_cstr)
		(*env)->ReleaseStringUTFChars(env, header, header_cstr);
}

void http_get(const char* addr, bool get_header, HttpCallback cb) {
	_http_check_init();

	JNIEnv* env = SDL_AndroidGetJNIEnv();
	jclass http_class = (*env)->FindClass(env, "com/quibble/dgreed/Http");
	jmethodID get = (*env)->GetStaticMethodID(
		env, http_class, "get", "(Ljava/lang/String;ZI)V"
	);

	int id = _register_cb(cb);

	jstring addr_str = (*env)->NewStringUTF(env, addr);
	(*env)->CallStaticVoidMethod(env, http_class, get, addr_str, get_header, id);	
	(*env)->DeleteLocalRef(env, addr_str);
}

void http_post(const char* addr, bool get_header,
	const char* data, const char* content_type, HttpCallback cb) {

	_http_check_init();

	JNIEnv* env = SDL_AndroidGetJNIEnv();
	jclass http_class = (*env)->FindClass(env, "com/quibble/dgreed/Http");
	jmethodID get = (*env)->GetStaticMethodID(
		env, http_class, "post", "(Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;I)V"
	);

	int id = _register_cb(cb);

	jstring addr_str = (*env)->NewStringUTF(env, addr);
	jstring data_str = (*env)->NewStringUTF(env, data);
	jstring content_type_str = (*env)->NewStringUTF(env, content_type);

	(*env)->CallStaticVoidMethod(env, http_class, get, 
		addr_str, get_header, data_str, content_type_str, id
	);	

	(*env)->DeleteLocalRef(env, addr_str);
	(*env)->DeleteLocalRef(env, data_str);
	(*env)->DeleteLocalRef(env, content_type_str);
}

void http_put(const char* addr, void* data, size_t size, bool get_header, HttpCallback cb) {

	_http_check_init();

	JNIEnv* env = SDL_AndroidGetJNIEnv();
	jclass http_class = (*env)->FindClass(env, "com/quibble/dgreed/Http");
	jmethodID get = (*env)->GetStaticMethodID(
		env, http_class, "put", "(Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;I)V"
	);

	int id = _register_cb(cb);

	const char* content_type = "application/octet-stream";

	jstring addr_str = (*env)->NewStringUTF(env, addr);
	// This won't work non-textual data, oh well ...
	jstring data_str = (*env)->NewStringUTF(env, (const char*)data);
	jstring content_type_str = (*env)->NewStringUTF(env, content_type);

	(*env)->CallStaticVoidMethod(env, http_class, get, 
		addr_str, get_header, data_str, content_type_str, id
	);	

	(*env)->DeleteLocalRef(env, addr_str);
	(*env)->DeleteLocalRef(env, data_str);
	(*env)->DeleteLocalRef(env, content_type_str);
}

void http_delete(const char* addr, bool get_header, HttpCallback cb) {
	_http_check_init();

	JNIEnv* env = SDL_AndroidGetJNIEnv();
	jclass http_class = (*env)->FindClass(env, "com/quibble/dgreed/Http");
	jmethodID get = (*env)->GetStaticMethodID(
		env, http_class, "delete", "(Ljava/lang/String;ZI)V"
	);

	int id = _register_cb(cb);

	jstring addr_str = (*env)->NewStringUTF(env, addr);
	(*env)->CallStaticVoidMethod(env, http_class, get, addr_str, get_header, id);	
	(*env)->DeleteLocalRef(env, addr_str);
}

