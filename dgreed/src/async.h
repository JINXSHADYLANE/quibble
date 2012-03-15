#ifndef ASYNC_H
#define ASYNC_H

#include "utils.h"

typedef uint CriticalSection;
typedef uint TaskId;
typedef void (*Task)(void*);

// Critical sections
CriticalSection async_make_cs(void);
void async_enter_cs(CriticalSection cs);
void async_leave_cs(CriticalSection cs);

// Run task asynchronously (might run on a different thread)
TaskId async_run(Task task, void* userdata);

// Run task asynchronously on a special io thread. All io tasks are
// executed in-order.
TaskId async_run_io(Task task, void* userdata);

// Schedule task to be executed in t miliseconds on the main thread.
// Timing is precise to 1/60 of a second.
TaskId async_schedule(Task task, uint t, void* userdata);

// Returns true if task is finished
bool async_is_finished(TaskId id);

#endif
