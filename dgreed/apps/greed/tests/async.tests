#include "async.h"

CriticalSection cs;

int fib(int n) {
	if(n == 0 || n == 1)
		return 1;
	if(n > 1)
		return fib(n-1) + fib(n-2);
	return -1;
}

void inc_counter(void* userdata) {
	int* counter = userdata;
	async_enter_cs(cs);
	*counter += 1;
	async_leave_cs(cs);
}

SETUP_{
	cs = async_make_cs();
}

void task_a(void* userdata) {
	int* dest = userdata;
	*dest = fib(10);
}

void task_b(void* userdata) {
	int* dest = userdata;
	*dest = fib(11);
}

TEST_(simple) {
	int ar = 0;
	int br = 0;

	TaskId a = async_run(task_a, &ar);
	TaskId b = async_run(task_b, &br);

	int res_a = fib(10);
	int res_b = fib(11);

	ASSERT_(fib(32) == 3524578);

	bool all_finished;
	do {
		all_finished = async_is_finished(a) && async_is_finished(b);
	} while(!all_finished);

	ASSERT_(ar == res_a && br == res_b);

	a = async_run(task_a, &br);
	while(!async_is_finished(a));
	ASSERT_(br = res_a);

	b = async_run(task_b, &ar);
	while(!async_is_finished(b));
	ASSERT_(ar = res_b);
}

TEST_(stress) {
	TaskId taskids[1000];
	int counter = 0;
	for(uint i = 0; i < 1000; ++i) {
		taskids[i] = async_run(inc_counter, &counter);
	}

	// Spin processor for a while
	int fib32 = fib(32);
	ASSERT_(fib32 == 3524578);

	// Wait for all tasks to finish
	bool all_finished;
	do {
		all_finished = true;
		for(uint i = 0; i < 1000; ++i) {
			if(!async_is_finished(taskids[i])) {
				all_finished = false;
				break;
			}
		}
	} while(!all_finished);

	ASSERT_(counter == 1000);
}


char* io_test_message = "kukutis";
char io_test_reversed[10];
char* io_write_ptr = io_test_reversed;

void io_test_task(void* userdata) {
	char* c = userdata;
	*io_write_ptr = *c;
	io_write_ptr++;
}

TEST_(io) {
	TaskId taskid = 0;
	uint l = strlen(io_test_message);
	for(uint i = 0; i < l; ++i) {
		taskid = async_run_io(io_test_task, &io_test_message[l-i-1]);	
	}
	io_test_reversed[l] = '\0';

	// Wait for the last task to finish
	while(!async_is_finished(taskid));

	ASSERT_(strcmp(io_test_reversed, "situkuk") == 0);
}
