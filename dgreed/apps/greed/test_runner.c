#include <stdio.h>
#include <stdbool.h>

#include "memory.h"
#include "utils.h"

// #define STOP_ON_ERROR
#define MAX_FAILED_ASSERTS 5

#ifdef STOP_ON_ERROR
#define STOP return;
#else
#define STOP
#endif

#define ASSERT__(group_name, test_name, file, line, expr) { \
	if(!can_assert) { \
		printf(" Unexpected assertion in %s:%d\n", file, line); \
	} \
	if(!(expr)) { \
		failed_asserts++; \
		if(failed_asserts > MAX_FAILED_ASSERTS) { \
			printf(" Too many failed asserts in %s::%s, skipping the rest.\n", \
				#group_name, #test_name); \
			return; \
		} \
		if(first_fail) \
			printf("\n"); \
		printf(" Assert failed in %s::%s, line %d\n", #group_name, \
			#test_name, line); \
		test_failed = true; \
		first_fail = false; \
		STOP \
	} \
}	

int failed_asserts;
bool test_failed;
bool first_fail;
int passed_tests;
bool can_assert = false;

typedef void (*fun_ptr)(void);

typedef struct {
	const char* name;
	fun_ptr setup;
	fun_ptr teardown;
	int first_test_idx;
	int test_count;
} TestGroup;	

// TestGroup groups[GROUP_COUNT];
// fun_ptr tests[TEST_COUNT];

#ifdef _DEBUG
#include "test_datad.h"
#else
#include "test_data.h"
#endif

void run_tests(void)
{
	TestGroup* current_group;
	int i, j;
	
	for(i = 0; i < GROUP_COUNT; ++i) {
		current_group = &groups[i];
		printf("Testing group %s ...", current_group->name);

		if(current_group->setup)
			(*current_group->setup)();

		int group_passed_tests = 0;
		first_fail = true;
		for(j = 0; j < current_group->test_count; ++j) {
			test_failed = false;

			can_assert = true;
			failed_asserts = 0;
			(*tests[current_group->first_test_idx + j])();
			can_assert = false;

			if(!test_failed) { 
				passed_tests++;
				group_passed_tests++;
			}	
			else {
			#ifdef STOP_ON_ERROR
				printf("Aborting further tests.\n");
				return;
			#endif	
			}
		}

		if(current_group->teardown)
			(*current_group->teardown)();

		if(group_passed_tests == current_group->test_count)
			printf(" all passed\n");
		else
			printf(" %d of %d passed\n", group_passed_tests, 
				current_group->test_count);
	}

	if(passed_tests == TEST_COUNT) 
		printf("All %d tests passed.\n", TEST_COUNT);
	else
		printf("There were failures. %d out of %d total tests passed\n",
			passed_tests, TEST_COUNT);
}

int dgreed_main(int argc, const char** argv) {
	run_tests();

	#ifdef TRACK_MEMORY
	log_init("tests.log", LOG_LEVEL_INFO);
	MemoryStats stats;
	mem_stats(&stats);
	LOG_INFO("Memory usage stats:");
	LOG_INFO(" Total allocations: %u", stats.n_allocations);
	LOG_INFO(" Peak dynamic memory usage: %uB", stats.peak_bytes_allocated);
	if(stats.bytes_allocated) {
		LOG_INFO(" Bytes still allocted: %u", stats.bytes_allocated);
		LOG_INFO(" Dumping allocations info to memory.txt");
		mem_dump("memory.txt");
	}	
	log_close();
	#endif

	if(passed_tests == TEST_COUNT)
		return 0;
	else
		return -1;
}		

