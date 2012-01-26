#include "async.h"
#include "darray.h"
#include "datastruct.h"
#include "system.h"

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

static bool async_initialized = false;

#define MAX_CRITICAL_SECTIONS 16
static pthread_mutex_t critical_sections[MAX_CRITICAL_SECTIONS];
static uint n_critical_sections = 4; // Critical sections 0..3 are used for async system

#define ASYNC_MKCS_CS 0
#define ASYNC_TASK_STATE_CS 1
#define ASYNC_SCHED_CS 2
#define ASYNC_THREAD_CS 3

#define THREAD_NAME_LEN 8
#define MAX_THREADS 4
#define ASYNC_THREADS 1
#define IO_THREAD 0

typedef struct {
	Task task;
	TaskId id;
	void* userdata;
	ListHead list;
} TaskDef;

typedef struct {
	ListHead queue;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int count;
} TaskQueue;

typedef struct {
	char name[THREAD_NAME_LEN];
	bool alive;
	pthread_t thread;
	TaskQueue* tq;
} WorkerThread;

static bool async_threads_created;
static bool io_thread_created;

static WorkerThread threads[MAX_THREADS];
static uint n_threads;

static void _async_init_task_state(void);
static void _async_close_task_state(void);
static void _async_init_queues(void);
static void _async_close_queues(void);
static void _async_stop_queues(void);
static void _async_init_scheduler(void);
static void _async_close_scheduler(void);

static void _check_async_threads(void);
static void _check_io_thread(void);

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
	async_initialized = true;

	n_threads = 0;
	async_threads_created = false;
	io_thread_created = false;

	_async_init_task_state();
	_async_init_queues();
	_async_init_scheduler();
}

void _async_close(void) {
	assert(async_initialized);

	_async_stop_queues();

	_async_close_scheduler();
	_async_close_queues();
	_async_close_task_state();

	for(uint i = 0; i > MAX_CRITICAL_SECTIONS; ++i) {
		pthread_mutex_destroy(&critical_sections[i]);
	}
}

const char* _async_thread_name(void) {
	pthread_t self = pthread_self();

	async_enter_cs(ASYNC_THREAD_CS);
	for(uint i = 0; i < n_threads; ++i) {
		if(pthread_equal(self, threads[i].thread)) {
			async_leave_cs(ASYNC_THREAD_CS);
			return threads[i].name;
		}
	}
	async_leave_cs(ASYNC_THREAD_CS);

	return "main";
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
	if(pthread_mutex_lock(&critical_sections[cs]) == EDEADLK) {
		LOG_ERROR("Deadlock in thread %s", _async_thread_name());
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


// Task queues

TaskQueue tq_async;
TaskQueue tq_io;

DArray taskdef_pool;
Heap taskdef_pool_freecells;

static void _async_init_task_queue(TaskQueue* tq) {
	assert(tq);

	list_init(&tq->queue);
	pthread_cond_init(&tq->cond, NULL);
	pthread_mutex_init(&tq->mutex, NULL);
	tq->count = 0;
}

static void _async_close_task_queue(TaskQueue* tq) {
	assert(tq);
	assert(list_empty(&tq->queue));
	assert(tq->count == -1);

	pthread_mutex_destroy(&tq->mutex);
	pthread_cond_destroy(&tq->cond);
}

static void _async_init_queues(void) {
	_async_init_task_queue(&tq_async);
	_async_init_task_queue(&tq_io);

	taskdef_pool = darray_create(sizeof(TaskDef), 0);
	heap_init(&taskdef_pool_freecells);
}

static void _async_stop_queues(void) {
	LOG_INFO("Stopping worker threads...");

	async_enter_cs(ASYNC_THREAD_CS);

	// Just set count to -1 and signal workers,
	// they will quit

	if(io_thread_created) {
		pthread_mutex_lock(&tq_io.mutex);
		tq_io.count = -1;
		pthread_mutex_unlock(&tq_io.mutex);
		pthread_cond_broadcast(&tq_io.cond);
	}
	
	if(async_threads_created) {
		pthread_mutex_lock(&tq_async.mutex);
		tq_async.count = -1;
		pthread_mutex_unlock(&tq_async.mutex);
		pthread_cond_broadcast(&tq_async.cond);
	}
	
	// Join all threads
	for(uint i = 0; i < n_threads; ++i) {
		int res = pthread_join(threads[i].thread, NULL);
		if(res != 0)
			LOG_ERROR("Unable to join thread %s", threads[i].name);
	}

	async_leave_cs(ASYNC_THREAD_CS);
}

static void _async_close_queues(void) {
	_async_close_task_queue(&tq_io);
	_async_close_task_queue(&tq_async);

	darray_free(&taskdef_pool);
	heap_free(&taskdef_pool_freecells);
}

// If append to taskdef pool triggers realloc, all pointers inside lists
// become invalid. Good news is that they all move by constant amount, so
// just recalculate all pointers inside lists.
static void _async_rebase_lists(ptrdiff_t delta) {
	void* range_start = taskdef_pool.data - delta;
	void* range_end = range_start + taskdef_pool.item_size * taskdef_pool.size;

	void* prev;
	void* next;

	if(!list_empty(&tq_io.queue)) {
		prev = (void*)tq_io.queue.prev;
		next = (void*)tq_io.queue.next;
		assert(range_start <= prev && prev < range_end);
		assert(range_start <= next && next < range_end);
		
		tq_io.queue.prev = (void*)(prev + delta);
		tq_io.queue.next = (void*)(next + delta);
	}

	if(!list_empty(&tq_async.queue)) {
		prev = (void*)tq_async.queue.prev;
		next = (void*)tq_async.queue.next;
		assert(range_start <= prev && prev < range_end);
		assert(range_start <= next && next < range_end);
		
		tq_async.queue.prev = (void*)(prev + delta);
		tq_async.queue.next = (void*)(next + delta);
	}

	TaskDef* list_nodes = DARRAY_DATA_PTR(taskdef_pool, TaskDef);
	for(uint i = 0; i < taskdef_pool.size; ++i) {
		TaskDef* def = &list_nodes[i];
		prev = (void*)def->list.prev;
		next = (void*)def->list.next;
		if(range_start <= prev && prev < range_end)
			def->list.prev = (ListHead*)(prev + delta);
		if(range_start <= next && next < range_end)
			def->list.next = (ListHead*)(next + delta);
	}
}

static void _async_enqueue(ListHead* head, Task task, TaskId id, void* userdata) {
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
		TaskDef dummy = {NULL, 0, NULL, {NULL, NULL}};
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
	new->id = id;
	new->userdata = userdata;

	list_push_back(head, &new->list);
}

static bool _async_dequeue(ListHead* head, TaskDef* dest) {
	if(list_empty(head))
		return false;

	TaskDef* first = list_entry(list_pop_front(head), TaskDef, list); 

	// Calculate taskdef index inside taskdef pool
	uint i = ((void*)first - taskdef_pool.data) / taskdef_pool.item_size;
	assert(i < taskdef_pool.size);

	// Mark cell as free
	heap_push(&taskdef_pool_freecells, i, NULL);

	*dest = *first;
	return true;
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
		async_leave_cs(ASYNC_SCHED_CS);
		(*def->task)(def->userdata);
		async_enter_cs(ASYNC_SCHED_CS);

		// Remove it from schedule task pool
		heap_push(&async_sched_freecells, i, NULL);

		// Mark taskid as finished
		_async_finish_taskid(def->taskid);
	}

	async_leave_cs(ASYNC_SCHED_CS);
}

// Threads

static void* _worker(void* userdata) {
	WorkerThread* self = (WorkerThread*)userdata;

	LOG_INFO("Thread %s starting work\n", self->name);
	self->alive = true;
	while(true) {
		TaskDef task;
		bool have_task = false;

		pthread_mutex_lock(&self->tq->mutex);
again:
		if(self->tq->count == 0) {
			// Wait till there's a task available
			pthread_cond_wait(&self->tq->cond, &self->tq->mutex);
			goto again;
		}
		if(self->tq->count > 0) {
				// Get the task
				have_task = _async_dequeue(&self->tq->queue, &task);
				self->tq->count--;
		}

		pthread_mutex_unlock(&self->tq->mutex);

		if(!have_task) {
			// We got signal, but there's no tasks - exit
			LOG_INFO("Thread %s exiting", self->name);
			break;
		}

		// Do the task
		(*task.task)(task.userdata);

		// Mark finished
		_async_finish_taskid(task.id);
	}
	self->alive = false;
	return NULL;
}

static void _create_thread(const char* name, TaskQueue* queue) {
	assert(async_initialized);
		
	WorkerThread* thread = &threads[n_threads];
	n_threads++;
	assert(n_threads < MAX_THREADS);

	assert(strlen(name) < THREAD_NAME_LEN);
	strcpy(thread->name, name);
	thread->tq = queue;
	thread->alive = false;

	int ret = pthread_create(&thread->thread, NULL, _worker, (void*)thread);
	if(ret != 0)
		LOG_ERROR("Unable to create worker thread");

	LOG_INFO("Created thread %s", name);
}

static void _check_io_thread(void) {
	async_enter_cs(ASYNC_THREAD_CS);
	if(!io_thread_created) {
		_create_thread("io", &tq_io);
		io_thread_created = true;
	}
	async_leave_cs(ASYNC_THREAD_CS);
}

static void _check_async_threads(void) {
	async_enter_cs(ASYNC_THREAD_CS);
	if(!async_threads_created) {
		char name[8];
		for(uint i = 0; i < ASYNC_THREADS; ++i) {
			sprintf(name, "async %d", i);
			_create_thread(name, &tq_async);
		}
		async_threads_created = true;
	}
	async_leave_cs(ASYNC_THREAD_CS);
}

TaskId async_run(Task task, void* userdata) {
	_check_async_threads();

	TaskId id = _async_new_taskid();

	pthread_mutex_lock(&tq_async.mutex);
	_async_enqueue(&tq_async.queue, task, id, userdata);
	tq_async.count++;
	pthread_mutex_unlock(&tq_async.mutex);
	pthread_cond_signal(&tq_async.cond);  

	return id;
}

TaskId async_run_io(Task task, void* userdata) {
	_check_io_thread();

	TaskId id = _async_new_taskid();

	pthread_mutex_lock(&tq_io.mutex);
	_async_enqueue(&tq_io.queue, task, id, userdata);
	tq_io.count++;
	pthread_mutex_unlock(&tq_io.mutex);
	pthread_cond_signal(&tq_io.cond);

	return id;
}

