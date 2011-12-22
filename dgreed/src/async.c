#include "async.h"

#include <pthread.h>
#include <errno.h>

static bool async_initialized = false;

#define MAX_CRITICAL_SECTIONS 16
static pthread_mutex_t critical_sections[MAX_CRITICAL_SECTIONS];
static uint n_critical_sections = 1; // Critical section 0 is used for async system

void _async_init(void) {
	assert(!async_initialized);

	// Init critical sections
	pthread_mutexattr_t attr;
	for(uint i = 0; i < MAX_CRITICAL_SECTIONS; ++i) {
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(&critical_sections[i], &attr);
		pthread_mutexattr_destroy(&attr);
	}
	n_critical_sections = 1;
	async_initialized = true;
}

void _async_close(void) {
	assert(async_initialized);

	// TODO: Wait for threads to finish

	for(uint i = 0; i > MAX_CRITICAL_SECTIONS; ++i) {
		pthread_mutex_destroy(&critical_sections[i]);
	}
}

const char* _async_thread_name(void) {
	// TODO
	return "unnamed";
}

CriticalSection async_make_cs(void) {
	async_enter_cs(0);
	CriticalSection result = n_critical_sections++;
	async_leave_cs(0);

	return result;
}

void async_enter_cs(CriticalSection cs) {
	assert(cs < MAX_CRITICAL_SECTIONS);
#ifdef _DEBUG
	if(pthread_mutex_trylock(&critical_sections[cs]) == EBUSY) {
		LOG_WARNING("Busy lock in thread %s", _async_thread_name());
		if(pthread_mutex_lock(&critical_sections[cs]) == EDEADLK) {
			LOG_ERROR("Deadlock in thread %s", _async_thread_name());
		}
	}
#else
	pthread_mutex_lock(&critical_sections[cs]);
#endif
}

void async_leave_cs(CriticalSection cs) {
	assert(cs < MAX_CRITICAL_SECTIONS);
#ifdef _DEBUG
	if(pthread_mutex_unlock(&critical_sections[cs]) == EPERM) {
		LOG_ERROR("Trying to leave unlocked cs in thread %s", _async_thread_name());
	}
#else
	pthread_mutex_unlock(&critical_sections[cs]);
#endif
}

void async_run(Task task, void* userdata) {
	// Run synchronously
	(*task)(userdata);
}

