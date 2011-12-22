#ifndef ASYNC_H
#define ASYNC_H

#include "utils.h"

typedef uint CriticalSection;
typedef void (*Task)(void*);

// Critical sections
CriticalSection async_make_cs(void);
void async_enter_cs(CriticalSection cs);
void async_leave_cs(CriticalSection cs);

// Run task asynchronously (might run on a different thread)
void async_run(Task task, void* userdata);

// Run task asynchronously on a special io thread. All io tasks are
// executed in-order.
void async_run_io(Task task, void* userdata);

// Schedule task to be executed in t miliseconds on the same thread.
// Timing is precise to 1/60 of a second.
void async_schedule(Task task, int t, void* userdata);

#endif
