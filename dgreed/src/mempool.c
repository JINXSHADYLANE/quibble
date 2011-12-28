#include "mempool.h"
#include "memory.h"

#define DEFAULT_CHUNK_ITEMS 256

#if x86_64 
	#define BITARRAY_TYPE uint64
	#define BITARRAY_BITS 64 
	#define BITARRAY_EMPTY 0xffffffffffffffffU

	#define BITARRAY_GET(arr, i)  \
		(arr[i / BITARRAY_BITS] & (1ULL << (i % BITARRAY_BITS)))

	#define BITARRAY_SET(arr, i) \
		arr[i / BITARRAY_BITS] |= (1ULL << (i % BITARRAY_BITS))

	#define BITARRAY_CLEAR(arr, i) \
		arr[i / BITARRAY_BITS] &= ~(1ULL << (i % BITARRAY_BITS))

#else
	#define BITARRAY_TYPE uint32
	#define BITARRAY_BITS 32
	#define BITARRAY_EMPTY 0xffffffffU

	#define BITARRAY_GET(arr, i)  \
		(arr[i / BITARRAY_BITS] & (1U << (i % BITARRAY_BITS)))

	#define BITARRAY_SET(arr, i) \
		arr[i / BITARRAY_BITS] |= (1U << (i % BITARRAY_BITS))

	#define BITARRAY_CLEAR(arr, i) \
		arr[i / BITARRAY_BITS] &= ~(1U << (i % BITARRAY_BITS))

#endif

typedef struct {
	void* data; 
	BITARRAY_TYPE* free; // Bitarray of cell occupation status
	int count;	  // Number of occupied cells
	ListHead list;// List of chunks
} MemPoolChunk;

void mempool_init(MemPool* pool, size_t item_size) {
	mempool_init_ex(pool, item_size, item_size * DEFAULT_CHUNK_ITEMS);
}

void mempool_init_ex(MemPool* pool, size_t item_size, size_t chunk_size) {
	assert(pool);
	assert(item_size);
	assert(chunk_size / item_size >= BITARRAY_BITS);

	pool->item_size = item_size;
	pool->chunk_size = (chunk_size / item_size) * item_size;
	list_init(&pool->chunks);
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
		if(pos->count < items_per_chunk) {
			chunk = pos;
			break;
		}
	}

	// No luck, alloc new chunk
	if(!chunk) {
		size_t header_size = sizeof(MemPoolChunk);
		size_t bitarray_size = items_per_chunk / BITARRAY_BITS * sizeof(BITARRAY_TYPE); 
		size_t size = header_size + bitarray_size + pool->chunk_size;

		chunk = MEM_ALLOC(size);
		chunk->free = (void*)chunk + header_size;
		chunk->data = (void*)chunk + header_size + bitarray_size;
		
		chunk->count = 0;

		// Reset free cells bitarray
		for(uint i = 0; i < items_per_chunk / BITARRAY_BITS; ++i)
			chunk->free[i] = BITARRAY_EMPTY;

		// Add chunk to the list
		list_push_back(&pool->chunks, &chunk->list);
	}

	// Find a free cell in the chunk, mark it occupied
	uint i = 0, idx = ~0;
	while(	i < items_per_chunk && 
			chunk->free[i / BITARRAY_BITS] == ~BITARRAY_EMPTY) {
	
		i += BITARRAY_BITS;
	}
	for(; i < items_per_chunk; ++i) {
		if(BITARRAY_GET(chunk->free, i)) {
			BITARRAY_CLEAR(chunk->free, i);
			idx = i;
			break;
		}
	}
	assert(idx < items_per_chunk);

	// Increase count, return cell address
	chunk->count++;
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
	assert(chunk->count);
	assert(chunk->data <= ptr && ptr < chunk->data + pool->chunk_size);

	// Calculate cell index
	ptrdiff_t offset = ptr - chunk->data;
	assert(offset % pool->item_size == 0);
	uint idx = offset / pool->item_size;
	assert(!BITARRAY_GET(chunk->free, idx));

	// Mark cell free, decrease chunk item count
	BITARRAY_SET(chunk->free, idx);
	chunk->count--;
}

