#include "mempool.h"
#include "memory.h"

#define DEFAULT_CHUNK_SIZE (32*1024) 

typedef struct {
	void* data; 
	int freelist_head;
	ListHead list; // List of chunks
} MemPoolChunk;

void mempool_init(MemPool* pool, size_t item_size) {
	mempool_init_ex(pool, item_size, DEFAULT_CHUNK_SIZE);
}

void mempool_init_ex(MemPool* pool, size_t item_size, size_t chunk_size) {
	assert(pool);
	assert(item_size);
	assert(item_size >= sizeof(int));

	pool->item_size = item_size;
	pool->chunk_size = (chunk_size / item_size) * item_size;
	list_init(&pool->chunks);
}

void mempool_free_all(MemPool* pool) {
	size_t items_per_chunk = pool->chunk_size / pool->item_size;
	MemPoolChunk* pos;
	list_for_each_entry(pos, &pool->chunks, list) {
		// Reset freelist
		pos->freelist_head = 0;
		for(uint i = 0; i < items_per_chunk; ++i) {
			int* freelist_entry = pos->data + pool->item_size * i;
			*freelist_entry = (i == items_per_chunk-1) ? -1 : i + 1;
		}
	}
}

void mempool_drain(MemPool* pool) {
	assert(pool);

	while(!list_empty(&pool->chunks)) {
		ListHead* back = list_pop_back(&pool->chunks);
		MemPoolChunk* chunk = list_entry(back, MemPoolChunk, list);
		MEM_FREE(chunk);
	}
}

void* mempool_alloc(MemPool* pool) {
	assert(pool);

	size_t items_per_chunk = pool->chunk_size / pool->item_size;
	MemPoolChunk* chunk = NULL;

	// Try to find a chunk with free cells
	MemPoolChunk* pos;
	list_for_each_entry(pos, &pool->chunks, list) {
		if(pos->freelist_head != -1) {
			chunk = pos;
			break;
		}
	}

	// No luck, alloc new chunk
	if(!chunk) {
		size_t header_size = sizeof(MemPoolChunk);
		size_t size = header_size + pool->chunk_size;

		chunk = MEM_ALLOC(size);
		chunk->data = (void*)chunk + header_size;
		
		chunk->freelist_head = 0;

		// Init freelist 
		for(int i = 0; i < items_per_chunk; ++i) {
			int* freelist_entry = chunk->data + pool->item_size * i;
			*freelist_entry = (i == items_per_chunk-1) ? -1 : i+1;
		}	

		// Add chunk to the chunks list
		list_push_back(&pool->chunks, &chunk->list);
	}

	// Find a free cell in the chunk, mark it occupied
	size_t idx = chunk->freelist_head;
	assert(idx < items_per_chunk);
	int* freelist_entry = chunk->data + idx * pool->item_size;
	chunk->freelist_head = *freelist_entry;

	// Return cell address
	return chunk->data + idx * pool->item_size;
}

void mempool_free(MemPool* pool, void* ptr) {
	assert(pool && ptr);

	// Find the right chunk
	MemPoolChunk* chunk;
	list_for_each_entry(chunk, &pool->chunks, list) {
		if(chunk->data <= ptr && ptr < chunk->data + pool->chunk_size)
			break;
	}

	assert(chunk);
	assert(chunk->data <= ptr && ptr < chunk->data + pool->chunk_size);

	// Calculate cell index
	ptrdiff_t offset = ptr - chunk->data;
	assert(offset % pool->item_size == 0);
	uint idx = offset / pool->item_size;

	// Mark cell free
	int* freelist_entry = chunk->data + idx * pool->item_size;
	*freelist_entry = chunk->freelist_head;
	chunk->freelist_head = idx;
}

bool mempool_owner(MemPool* pool, void* ptr) {
	// Find the right chunk
	MemPoolChunk* chunk;
	list_for_each_entry(chunk, &pool->chunks, list) {
		if(chunk->data <= ptr && ptr < chunk->data + pool->chunk_size)
			return true;
	}
	return false;
}

