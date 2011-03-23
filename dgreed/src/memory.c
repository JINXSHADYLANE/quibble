#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "memory.h"

#define MAX_ALLOCATIONS 4096

typedef struct {
	unsigned int id;
	unsigned int inv_id;
} MemBlockHeader;

typedef struct {
	void* ptr;
	size_t size;

	const char* file;
	unsigned int line;
} MemAllocation;

MemAllocation allocations[MAX_ALLOCATIONS];
size_t allocations_count = 0;

MemoryStats stats = {0, 0, 0, 0};

void* mem_alloc(size_t size, const char* file, int line) {
	// Actual memory allocation
	void* ptr = malloc(size + sizeof(MemBlockHeader));
	
	assert(size);
	assert(allocations_count < MAX_ALLOCATIONS);
	assert(ptr);

	// Fill new MemAllocation struct
	allocations[allocations_count].ptr = ptr;
	allocations[allocations_count].size = size;
	allocations[allocations_count].file = file;
	allocations[allocations_count].line = line;

	allocations_count++;

	// Update stats
	stats.bytes_allocated += size;
	if(stats.bytes_allocated > stats.peak_bytes_allocated)
		stats.peak_bytes_allocated = stats.bytes_allocated;
	stats.n_allocations++;

	// Fill header
	MemBlockHeader* header = ptr;
	header->id = allocations_count-1;
	header->inv_id = ~header->id;

	return ptr + sizeof(MemBlockHeader);
}

void* mem_calloc(size_t num, size_t size, const char* file, int line) {
	assert(num);
	assert(size);

	void* ptr = mem_alloc(num * size, file, line);
	memset(ptr, 0, num * size);
	return ptr;
}

void* mem_realloc(void* p, size_t size, const char* file, int line) {
	void* ptr;
	unsigned int i;
	
	assert(size);
	assert(allocations_count < MAX_ALLOCATIONS);

	// Find right MemAllocation struct
	size_t header_size = sizeof(MemBlockHeader);
	MemBlockHeader* header = p - header_size;
	assert(header->id == ~header->inv_id);
	i = header->id;
	assert(p - header_size == allocations[i].ptr);

	// Reallocate old memory
	ptr = (void*)realloc(p - header_size, size + header_size);
	assert(ptr);

	// Subtract old alloc size
	stats.bytes_allocated -= allocations[i].size;

	// Update allocation info
	allocations[i].ptr = ptr;
	allocations[i].size = size;
	allocations[i].file = file;
	allocations[i].line = line;

	// Update stats 
	stats.bytes_allocated += size;
	if(stats.bytes_allocated > stats.peak_bytes_allocated)
		stats.peak_bytes_allocated = stats.bytes_allocated;
	
	return ptr + header_size;
}

void mem_free(const void* ptr) {
	unsigned int i;

	assert(ptr);

	// Find right MemAllocation struct
	size_t header_size = sizeof(MemBlockHeader);
	MemBlockHeader* header = (void*)ptr - header_size;
	assert(header->id == ~header->inv_id);
	i = header->id;

	assert(ptr - header_size == allocations[i].ptr);

	// Quick way to remove item from array:
	// Copy last item to its place, decrease item count 
	size_t alloc_size = allocations[i].size;
	allocations[i] = allocations[allocations_count-1];
	allocations_count--;

	// Update previuosly-last allocation header
	MemBlockHeader* lheader = allocations[i].ptr;
	lheader->id = i;
	lheader->inv_id = ~lheader->id;

	// Free memory
	void* p = (void*)ptr - header_size;
	free(p);

	// Update stats
	stats.bytes_allocated -= alloc_size;
	stats.n_deallocations++;
}

void mem_stats(MemoryStats* mstats) {
	assert(mstats);

	memcpy(mstats, &stats, sizeof(MemoryStats));
}

void mem_dump(const char* path) {
	unsigned int i;
	
	// Dump to stderr on iOS
#ifdef TARGET_IOS
	FILE* output = stderr;
#else	
	FILE* output = fopen(path, "w");
#endif
	
	fprintf(output, "Memory allocations dump:\n");
	for(i = 0; i < allocations_count; ++i) {
		fprintf(output, " %u: %s:%u:\t%zu\t%p08\n", 
			i+1, allocations[i].file, allocations[i].line,
			allocations[i].size, allocations[i].ptr); 
	}
	fprintf(output, "Total memory allocated: %zu bytes\n",
		stats.bytes_allocated);
	fclose(output);
}

