#ifndef MEMLIN_H
#define MEMLIN_H

#include "utils.h"

// Linear memory allocator
// Useful for allocating lots of different size short-lived bits of memory.

typedef struct MemLinChunk {
	void* data;
	void* cursor;
	size_t free;
	struct MemLinChunk* next;
} MemLinChunk;

typedef struct {
	size_t chunk_size;
	MemLinChunk* list;
} MemLin;

void memlin_init(MemLin* lin, size_t chunk_size);
void memlin_drain(MemLin* lin);

void* memlin_alloc(MemLin* lin, size_t size);
void memlin_free(MemLin* lin, void* ptr, size_t size);

#endif
