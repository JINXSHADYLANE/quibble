#include "memlin.h"
#include "memory.h"

void memlin_init(MemLin* lin, size_t chunk_size) {
	assert(lin && chunk_size);

	lin->chunk_size = chunk_size;
	lin->list = NULL;
}

void memlin_drain(MemLin* lin) {
	assert(lin && lin->chunk_size);

	// Iterate over chunks, free each one
	MemLinChunk* chunk = lin->list;
	while(chunk) {
		void* ptr = chunk;
		chunk = chunk->next;
		MEM_FREE(ptr);	
	}
}

static MemLinChunk* _new_chunk(MemLin* lin) {
	// Alloc chunk header and the chunk itself in one memory block
	size_t s = lin->chunk_size + sizeof(MemLinChunk);
	MemLinChunk* chunk = MEM_ALLOC(s);
	chunk->data = (void*)chunk + sizeof(MemLinChunk);
	chunk->cursor = chunk->data;
	chunk->free = 0;

	// Put the new chunk at the list front so it will be
	// the first one checked for the free space
	chunk->next = lin->list;
	lin->list = chunk;

	return chunk;
}

void* memlin_alloc(MemLin* lin, size_t size) {
	assert(lin && size);
	assert(size < lin->chunk_size/2);

	// Find chunk with enough space
	MemLinChunk* chunk = NULL;
	MemLinChunk* itr = lin->list;
	while(itr) {
		size_t free_space = lin->chunk_size - (itr->cursor - itr->data);
		if(free_space >= size) {
			chunk = itr;
			break;
		}
		else {
			itr = itr->next;
		}
	}

	// Alloc new chunk if none found
	if(!chunk)
		chunk = _new_chunk(lin);

	// Allocate space by moving cursor pointer forwards
	void* ptr = chunk->cursor;
	chunk->cursor += size;
	return ptr;
}

void memlin_free(MemLin* lin, void* ptr, size_t size) {
	assert(lin && size);
	assert(size < lin->chunk_size/2);

	// Find chunk to whom the pointer belongs
	MemLinChunk* chunk = NULL;
	MemLinChunk* itr = lin->list;
	while(itr) {
		if(ptr >= itr->data && ptr < itr->cursor) {
			chunk = itr;
			break;
		}
		else {
			itr = itr->next;
		}
	}

	assert(chunk);

	if(chunk->cursor - size == ptr) {
		// If ptr is the last item - move cursor back
		chunk->cursor -= size;
	}
	else {
		// Otherwise just accumulate amount of freed space
		chunk->free += size;
	}

	// If all chunk became free, move the cursor back to the start
	if(chunk->free == (chunk->cursor - chunk->data)) {
		chunk->cursor = chunk->data;
		chunk->free = 0;
	}
}

