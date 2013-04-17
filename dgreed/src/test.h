#ifndef TEST_H
#define TEST_H

#include <utils.h>

typedef void (*fun_ptr)(void);

typedef struct {
	const char* name;
	fun_ptr setup;
	fun_ptr teardown;
	int first_test_idx;
	int test_count;
} TestGroup;	


extern int failed_asserts;
extern bool test_failed;
extern bool first_fail;
extern int passed_tests;
extern bool can_assert;

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

#endif
