#include "memory.h"

size_t m = 0;

SETUP_{
	MemoryStats stats;
	mem_stats(&stats);
	m = stats.bytes_allocated;
}

TEST_(alloc_and_free) {
	void* mem = NULL;
	void* mem2 = NULL;
	MemoryStats stats;

	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 0);

	mem = MEM_ALLOC(16);
	ASSERT_(mem);
	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 16);
	MEM_FREE(mem);
	mem = NULL;

	mem = MEM_ALLOC(32174);
	ASSERT_(mem);
	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 32174);

	mem2 = MEM_ALLOC(296874);
	ASSERT_(mem2);
	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 32174 + 296874);
	MEM_FREE(mem);
	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 296874);
	MEM_FREE(mem2);
	mem_stats(&stats);

	ASSERT_(stats.bytes_allocated - m == 0);
	
}

TEST_(peak_bytes) {
	MemoryStats stats;
	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 0);

	size_t peak_bytes = stats.peak_bytes_allocated;
	void* mem = MEM_ALLOC(peak_bytes + 1);
	ASSERT_(mem);
	MEM_FREE(mem);

	mem_stats(&stats);
	ASSERT_(stats.peak_bytes_allocated - m == peak_bytes + 1);
	ASSERT_(stats.bytes_allocated - m == 0);
}	

TEST_(realloc) {
	MemoryStats stats;
	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 0);

	void* mem = MEM_ALLOC(67);
	ASSERT_(mem);
	mem = MEM_REALLOC(mem, 431);
	mem_stats(&stats);
	ASSERT_(mem);
	ASSERT_(stats.bytes_allocated - m == 431);
	MEM_FREE(mem);

	mem_stats(&stats);
	ASSERT_(stats.bytes_allocated - m == 0);
}	

