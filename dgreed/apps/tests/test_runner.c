#include <stdio.h>
#include <stdbool.h>

#include "test.h"
#include "memory.h"
#include "utils.h"

int failed_asserts;
bool test_failed;
bool first_fail;
int passed_tests;
bool can_assert = false;

extern TestGroup groups[];
extern uint group_count;
extern fun_ptr tests[];
extern uint test_count;

void run_tests(void)
{
	TestGroup* current_group;
	int i, j;
	
	for(i = 0; i < group_count; ++i) {
		current_group = &groups[i];
		printf("Testing group %s ...", current_group->name);

		#ifdef TRACK_MEMORY
		MemoryStats stats;
		mem_stats(&stats);
		size_t mem_used = stats.bytes_allocated;
		#endif

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

		#ifdef TRACK_MEMORY
		mem_stats(&stats);
		if(stats.bytes_allocated != mem_used) {
			printf(" Leaked %zu bytes of memory!\n", stats.bytes_allocated - mem_used);
		}
		#endif

		if(group_passed_tests == current_group->test_count)
			printf(" all passed\n");
		else
			printf(" %d of %d passed\n", group_passed_tests, 
				current_group->test_count);
	}

	if(passed_tests == test_count) 
		printf("All %d tests passed.\n", test_count);
	else
		printf("There were failures. %d out of %d total tests passed\n",
			passed_tests, test_count);
}

int dgreed_main(int argc, const char** argv) {
	run_tests();

	if(passed_tests == test_count)
		return 0;
	else
		return -1;
}		

