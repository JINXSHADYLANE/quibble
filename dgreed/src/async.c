#include "async.h"
#include "darray.h"
#include "datastruct.h"
#include "system.h"

#include <pthread.h>
#include <errno.h>

static bool async_initialized = false;

#define MAX_CRITICAL_SECTIONS 16
static pthread_mutex_t critical_sections[MAX_CRITICAL_SECTIONS];
static uint n_critical_sections = 3; // Critical sections 0, 1 & 2 are used for async system

#define ASYNC_MKCS_CS 0
#define ASYNC_TASK_STATE_CS 1
#define ASYNC_SCHED_CS 2

static void _async_init_task_state(void);
static void _async_close_task_state(void);
static void _async_init_queues(void);
static void _async_close_queues(void);
static void _async_init_scheduler(void);
static void _async_close_scheduler(void);

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
	n_critical_sections = 3;
	async_initialized = true;

	_async_init_task_state();
	_async_init_queues();
	_async_init_scheduler();
}

void _async_close(void) {
	assert(async_initialized);

	_async_close_scheduler();
	_async_close_queues();
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
AATree async_unfinished_taskids;

static void _async_init_task_state(void) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);
	
	assert(!async_task_state_initialized);
	
	aatree_init(&async_unfinished_taskids);
	async_task_state_initialized = true;

	async_leave_cs(ASYNC_TASK_STATE_CS);
}

static void _async_close_task_state(void) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);

	assert(async_task_state_initialized);

	if(aatree_size(&async_unfinished_taskids) != 0) 
		LOG_WARNING("Closing task state tracker with unfinished tasks!");

	aatree_free(&async_unfinished_taskids);
	async_task_state_initialized = false;

	async_leave_cs(ASYNC_TASK_STATE_CS);
}

static TaskId _async_new_taskid(void) {
	assert(async_initialized);

	async_enter_cs(ASYNC_TASK_STATE_CS);

	assert(async_task_state_initialized);
	TaskId result = async_next_taskid++;
	aatree_insert(&async_unfinished_taskids, result, (void*)1);

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
	void* p = aatree_remove(&async_unfinished_taskids, id);
	assert(p);
	p = 0; // Prevent unused warning

	// Update highest finished taskid
	async_highest_finished_taskid = MAX(async_highest_finished_taskid, id);
	
	// If id was the previous lowest unfinished taskid, find a new one.
	// Also find a new one if lowest unfinished taskid is 0 (means previously 
	// there were no unfinished tasks).
	if(	id == async_lowest_unfinished_taskid ||
		0 == async_lowest_unfinished_taskid) {

		uint n_unfinished_tasks = aatree_size(&async_unfinished_taskids);

		// If no tasks are unfinished, set lowest taskid to 0
		if(0 == n_unfinished_tasks) {
			async_lowest_unfinished_taskid = 0;
		}
		else {
			async_lowest_unfinished_taskid = 
				aatree_min(&async_unfinished_taskids, NULL);
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
		void* p = aatree_find(&async_unfinished_taskids, id);
		result = (p == NULL);
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


// Task queues

typedef struct {
	Task task;
	void* userdata;
	ListHead list;
} TaskDef;

ListHead async_queue;
uint async_queue_count;
ListHead async_io_queue;
uint async_io_queue_count;

DArray taskdef_pool;
Heap taskdef_pool_freecells;

static void _async_init_queues(void) {
	list_init(&async_queue);
	list_init(&async_io_queue);
	taskdef_pool = darray_create(sizeof(TaskDef), 0);
	heap_init(&taskdef_pool_freecells);
	async_queue_count = 0;
	async_io_queue_count = 0;
}

static void _async_close_queues(void) {
	darray_free(&taskdef_pool);
	heap_free(&taskdef_pool_freecells);
}

// If append to taskdef pool triggers realloc, all pointers inside lists
// become invalid. Good news is that they all move by constant amount, so
// just recalculate all pointers inside lists.
static void _async_rebase_lists(ptrdiff_t delta) {
	void* range_start = taskdef_pool.data - delta;
	void* range_end = range_start + taskdef_pool.item_size * taskdef_pool.size;

	async_queue.next += delta;
	async_queue.prev += delta;
	async_io_queue.next += delta;
	async_io_queue.prev += delta;

	TaskDef* list_nodes = DARRAY_DATA_PTR(taskdef_pool, TaskDef);
	for(uint i = 0; i < taskdef_pool.size; ++i) {
		TaskDef* def = &list_nodes[i];
		void* prev = (void*)def->list.prev;
		void* next = (void*)def->list.next;
		if(range_start <= prev && prev < range_end)
			def->list.prev += delta;
		if(range_start <= next && next < range_end)
			def->list.next += delta;
	}
}

static void _async_enqueue(ListHead* head, Task task, void* userdata) {
	assert(head);
	assert(taskdef_pool.item_size == sizeof(TaskDef));

	uint i = ~0;
	if(heap_size(&taskdef_pool_freecells) > 0) {
		// We're lucky, there is a free cell
		i = heap_pop(&taskdef_pool_freecells, NULL);
	}
	else {
		// We have to append new cell, which might trigger realloc
		void* old_data = taskdef_pool.data;
		TaskDef dummy = {NULL, NULL, {NULL, NULL}};
		i = taskdef_pool.size;
		darray_append(&taskdef_pool, &dummy);
		void* new_data = taskdef_pool.data;
		if(old_data != new_data)
			_async_rebase_lists(new_data - old_data);
	}
	assert(i < taskdef_pool.size);

	// Fill data and push taskdef to the list
	TaskDef* defs = DARRAY_DATA_PTR(taskdef_pool, TaskDef);
	TaskDef* new = &defs[i];

	new->task = task;
	new->userdata = userdata;

	list_push_back(head, &new->list);
}

static TaskDef* _async_dequeue(ListHead* head) {
	if(list_empty(head))
		return NULL;

	TaskDef* first = list_entry(list_pop_front(head), TaskDef, list); 

	// Calculate taskdef index inside taskdef pool
	uint i = ((void*)first - taskdef_pool.data) / taskdef_pool.item_size;
	assert(i < taskdef_pool.size);

	// Mark cell as free
	heap_push(&taskdef_pool_freecells, i, NULL);

	return first;
}


// Scheduler

typedef struct {
	TaskId taskid;
	Task task;
	void* userdata;
} ScheduledTaskDef;

DArray async_sched_tasks;
Heap async_sched_freecells;
Heap async_schedule_pq;

// Current time in miliseconds
static int _async_time(void) {
	return time_ms_current();
}

static void _async_init_scheduler(void) {
	async_sched_tasks = darray_create(sizeof(ScheduledTaskDef), 0);
	heap_init(&async_sched_freecells);
	heap_init(&async_schedule_pq);
}

static void _async_close_scheduler(void) {
	if(heap_size(&async_schedule_pq) != 0) {
		LOG_WARNING("Closing scheduler with unfinished tasks!");
	}

	heap_free(&async_schedule_pq);
	heap_free(&async_sched_freecells);

	darray_free(&async_sched_tasks);
}

TaskId async_schedule(Task task, uint t, void* userdata) {
	assert(async_sched_tasks.item_size == sizeof(ScheduledTaskDef));

	TaskId taskid = _async_new_taskid();

	async_enter_cs(ASYNC_SCHED_CS);

	size_t i = ~0;
	if(heap_size(&async_sched_freecells) > 0) {
		// Use free cell
		i = heap_pop(&async_sched_freecells, NULL);
	}
	else {
		// Append new cell
		i = async_sched_tasks.size;
		ScheduledTaskDef dummy = {0, NULL, NULL};
		darray_append(&async_sched_tasks, &dummy);
	}
	assert(i != ~0);

	// Fill in data, push to the heap
	ScheduledTaskDef* defs = DARRAY_DATA_PTR(async_sched_tasks, ScheduledTaskDef);
	ScheduledTaskDef* new = &defs[i];

	new->taskid = taskid;
	new->task = task;
	new->userdata = userdata;

	int schedule_t = t + _async_time();
	heap_push(&async_schedule_pq, schedule_t, (void*)i);

	async_leave_cs(ASYNC_SCHED_CS);

	return taskid;
}

void async_process_schedule(void) {
	async_enter_cs(ASYNC_SCHED_CS);
	int t = _async_time();

	while(  heap_size(&async_schedule_pq) &&
			heap_peek(&async_schedule_pq, NULL) <= t) {

		// Pop highest priority task
		void* data;
		heap_pop(&async_schedule_pq, &data);
		size_t i = (size_t)data;
		assert(i < async_sched_tasks.size);
		ScheduledTaskDef* defs = DARRAY_DATA_PTR(async_sched_tasks, ScheduledTaskDef);
		ScheduledTaskDef* def = &defs[i];

		// Do it
		(*def->task)(def->userdata);

		// Remove it from schedule task pool
		heap_push(&async_sched_freecells, i, NULL);

		// Mark taskid as finished
		_async_finish_taskid(def->taskid);
	}

	async_leave_cs(ASYNC_SCHED_CS);
}

