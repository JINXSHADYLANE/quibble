#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "utils.h"
#include "datastruct.h"

// Memory pool allocator
// Useful for allocating many small objects of same size

typedef struct {
	size_t item_size;
	size_t chunk_size;
	ListHead chunks;
} MemPool;

void mempool_init(MemPool* pool, size_t item_size);
void mempool_init_ex(MemPool* pool, size_t item_size, size_t chunk_size);
void mempool_free_all(MemPool* pool);
void mempool_drain(MemPool* pool);

void* mempool_alloc(MemPool* pool);
void mempool_free(MemPool* pool, void* ptr);
bool mempool_owner(MemPool* pool, void* ptr);

#endif
