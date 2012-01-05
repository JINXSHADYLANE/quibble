#include "mempool.h"

// No explicit checks for memory leaks are done, testing harness does that 

TEST_(empty) {
	MemPool p;

	mempool_init(&p, sizeof(int));
	mempool_drain(&p);
}

TEST_(small) {
	MemPool p;

	int* m[100] = {NULL};

	mempool_init(&p, sizeof(int));

	for(uint i = 0; i < 100; ++i) {
		m[i] = mempool_alloc(&p);
		ASSERT_(m[i]);
		*m[i] = i;
	}

	for(uint i = 0; i < 50; ++i) {
		mempool_free(&p, m[i*2]);
	}

	for(uint i = 0; i < 100; ++i) {
		if(i % 2 == 1) {
			ASSERT_(*m[i] == i);
		}
		m[i] = mempool_alloc(&p);
		ASSERT_(m[i]);
	}

	mempool_drain(&p);
}

typedef struct {
	float x, y, z, w;
	void* next;
	void* prev;
} StructA;

TEST_(large) {
	MemPool p;

	mempool_init_ex(&p, sizeof(StructA), 4096);

	StructA* a[256];
	for(uint i = 0; i < 256; ++i) {
		a[i] = mempool_alloc(&p);
		ASSERT_(a[i]);
	}

	for(uint i = 255; i >= 32; --i) {
		mempool_free(&p, a[i]);	
	}

	StructA* b[256];
	for(uint i = 0; i < 256; ++i) {
		b[i] = mempool_alloc(&p);
		a[i] = mempool_alloc(&p);
		ASSERT_(a[i] && b[i]);
		if(i > 10)
			mempool_free(&p, b[i-10]);
	}

	mempool_drain(&p);
}

TEST_(stress) {
	MemPool p;

	mempool_init_ex(&p, sizeof(double), sizeof(double)*2048);

	double* m[10000];

	for(uint i = 0; i < 10000; ++i) {
		m[i] = mempool_alloc(&p);	
		ASSERT_(m[i]);
	}

	for(uint i = 0; i < 10; ++i) {
		for(uint j = 0; j < 10000; j += 3) {
			mempool_free(&p, m[j]);
		}

		for(uint j = 0; j < 10000; j++) {
			m[j] = mempool_alloc(&p);
		}
	}
	
	mempool_drain(&p);
}

