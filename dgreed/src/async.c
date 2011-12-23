#include "async.h"
#include "darray.h"

#include <pthread.h>
#include <errno.h>

static bool async_initialized = false;

#define MAX_CRITICAL_SECTIONS 16
static pthread_mutex_t critical_sections[MAX_CRITICAL_SECTIONS];
static uint n_critical_sections = 2; // Critical sections 0 & 1 are used for async system

#define ASYNC_MKCS_CS 0
#define ASYNC_TASK_STATE_CS 1

void _async_init_task_state(void);
void _async_close_task_state(void);

void _async_init(void) {
	assert(!async_initialized);

	// Init critical sections
	for(uint i = 0; i < MAX_CRITICAL_SECTIONS; ++i) {
#ifdef _DEBUG
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(&critical_sections[i], &attr);
		pthread_mutexattr_destroy(&attr);
#else
		pthread_mutex_init(&critical_sections[i], NULL);
#endif
	}
	n_critical_sections = 2;
	async_initialized = true;

	_async_init_task_state();
}

void _async_close(void) {
	assert(async_initialized);

	_async_close_task_state();

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
	assert(async_initialized);

	async_enter_cs(ASYNC_MKCS_CS);
	CriticalSection result = n_critical_sections++;
	async_leave_cs(ASYNC_MKCS_CS);

	return result;
}

void async_enter_cs(CriticalSection cs) {
	assert(async_initialized);
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
	assert(async_initialized);
	assert(cs < MAX_CRITICAL_SECTIONS);
#ifdef _DEBUG
	if(pthread_mutex_unlock(&critical_sections[cs]) == EPERM) {
		LOG_ERROR("Trying to leave unlocked cs in thread %s", _async_thread_name());
	}
#else
	pthread_mutex_unlock(&critical_sections[cs]);
#endif
}

// Task state tracking

bool async_task_state_initialized = false;
TaskId async_next_taskid = 1;
TaskId async_highest_finished_taskid = 0;
TaskId async_lowest_unfinished_taskid = 0;
DArray async_unfinished_taskids;

void _async_init_task_state(void) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);
	
	assert(!async_task_state_initialized);
	
	async_unfinished_taskids = darray_create(sizeof(uint), 0);
	async_task_state_initialized = true;

	async_leave_cs(ASYNC_TASK_STATE_CS);
}

void _async_close_task_state(void) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);

	assert(async_task_state_initialized);

	if(async_unfinished_taskids.size != 0) 
		LOG_ERROR("Closing task state tracker with unfinished tasks!");

	darray_free(&async_unfinished_taskids);
	async_task_state_initialized = false;

	async_leave_cs(ASYNC_TASK_STATE_CS);
}

static TaskId _async_new_taskid(void) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);

	assert(async_task_state_initialized);
	TaskId result = async_next_taskid++;
	darray_append(&async_unfinished_taskids, &result);

	async_leave_cs(ASYNC_TASK_STATE_CS);
	return result;
}

static void _async_finish_taskid(TaskId id) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);

	assert(async_task_state_initialized);
	assert(id < async_next_taskid);
	assert(id >= async_lowest_unfinished_taskid);

	// Remove id from unfinished taskids list
	uint taskid_index = ~0;
	uint* unfinished_taskids = DARRAY_DATA_PTR(async_unfinished_taskids, uint);
	for(uint i = 0; i < async_unfinished_taskids.size; ++i) {
		if(unfinished_taskids[i] == id) {
			taskid_index = i;
			break;
		}
	}
	assert(taskid_index != ~0);
	darray_remove_fast(&async_unfinished_taskids, taskid_index);

	// Update highest finished taskid
	async_highest_finished_taskid = MAX(async_highest_finished_taskid, id);
	
	// If id was the previous lowest unfinished taskid, find a new one.
	// Also find a new one if lowest unfinished taskid is 0 (means previously 
	// there were no unfinished tasks).
	if(	id == async_lowest_unfinished_taskid ||
		0 == async_lowest_unfinished_taskid) {

		uint n_unfinished_tasks = async_unfinished_taskids.size;

		// If no tasks are unfinished, set lowest taskid to 0
		if(0 == n_unfinished_tasks) {
			async_lowest_unfinished_taskid = 0;
		}
		else {
			uint* unfinished_taskids = DARRAY_DATA_PTR(async_unfinished_taskids, uint);
			TaskId min_taskid = unfinished_taskids[0];
			for(uint i = 1; i < async_unfinished_taskids.size; ++i) {
				min_taskid = MIN(min_taskid, unfinished_taskids[i]);
			}
			async_lowest_unfinished_taskid = min_taskid;
		}
	}
	
	async_leave_cs(ASYNC_TASK_STATE_CS);
}

bool async_is_finished(TaskId id) {
	assert(async_initialized);
	
	bool result;

	async_enter_cs(ASYNC_TASK_STATE_CS);
	assert(async_task_state_initialized);
	assert(id < async_next_taskid);

	if(id < async_lowest_unfinished_taskid) {
		result = true;
	}
	else if(id > async_highest_finished_taskid) {
		result = false;
	}
	else {
		// We need to check if taskid id is in unfinished set
		uint* unfinished_taskids = DARRAY_DATA_PTR(async_unfinished_taskids, uint);
		bool found = false;
		for(uint i = 0; i < async_unfinished_taskids.size; ++i) {
			if(unfinished_taskids[i] == id) {
				result = false;
				found = true;
				break;
			}
		}
		if(!found)
			result = true;
	}

	async_leave_cs(ASYNC_TASK_STATE_CS);

	return result;
}

TaskId async_run(Task task, void* userdata) {
	TaskId id = _async_new_taskid();

	// Run synchronously
	(*task)(userdata);

	_async_finish_taskid(id);

	return id;
}

TaskId async_run_io(Task task, void* userdata) {
	TaskId id = _async_new_taskid();

	// Run synchronously
	(*task)(userdata);

	_async_finish_taskid(id);

	return id;
}

