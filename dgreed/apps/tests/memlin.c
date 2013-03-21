#include "memlin.h"

// No explicit checks for memory leaks are done, testing harness does that 

TEST_(empty) {
	MemLin l;

	memlin_init(&l, 1024);
	memlin_drain(&l);
}

TEST_(small) {
	MemLin l;

	int* m[100] = {NULL};

	memlin_init(&l, 100 * sizeof(int));

	for(uint i = 0; i < 100; ++i) {
		m[i] = memlin_alloc(&l, sizeof(int));
		ASSERT_(m[i]);
		*m[i] = i;
	}

	for(uint i = 0; i < 50; ++i) {
		memlin_free(&l, m[i*2], sizeof(int));
	}

	for(uint i = 0; i < 100; ++i) {
		if(i % 2 == 1) {
			ASSERT_(*m[i] == i);
		}
		m[i] = memlin_alloc(&l, sizeof(int));
		ASSERT_(m[i]);
	}


	memlin_drain(&l);
}

TEST_(strings) {

	MemLin l;

	char* n[1000] = {NULL};

	memlin_init(&l, 4096);
 
	for(int i = 0; i < 1000; ++i) {
		char number[8];
		sprintf(number, "%d", i);
		size_t len = strlen(number);

		n[i] = memlin_alloc(&l, len+1);
		strcpy(n[i], number);
	}

	for(int i = 0; i < 1000; ++i) {
		char number[8];
		sprintf(number, "%d", i);
		size_t len = strlen(number);

		ASSERT_(strcmp(n[i], number) == 0);

		memlin_free(&l, n[i], len);
	}

	for(int i = 0; i < 1000; ++i) {
		char number[8];
		sprintf(number, "%d", i*15);
		size_t len = strlen(number);

		n[i] = memlin_alloc(&l, len+1);
		strcpy(n[i], number);
	}

	for(int i = 999; i >= 0; --i) {
		char number[8];
		sprintf(number, "%d", i*15);
		size_t len = strlen(number);

		ASSERT_(strcmp(n[i], number) == 0);

		memlin_free(&l, n[i], len);
	}

	memlin_drain(&l);

}

TEST_(stress) {

	MemLin l;
	memlin_init(&l, 8192);

	double* m[10000];
	
	for(uint i = 0; i < 10000; ++i) {
		m[i] = memlin_alloc(&l, sizeof(double));
		ASSERT_(m[i]);
	}

	for(uint i = 0; i < 10; ++i) {
		for(uint j = i%3; j < 10000; j += 3) {
			memlin_free(&l, m[j], sizeof(double));
		}

		for(uint j = 0; j < 10000; j++) {
			m[j] = memlin_alloc(&l, sizeof(double));
		}
	}
	
	memlin_drain(&l);

}

