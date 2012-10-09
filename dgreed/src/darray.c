#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "darray.h"
#include "memory.h"

#ifdef TARGET_IOS
	// Try not to allocate less than 1/4 k
	#define MINIMAL_CHUNK 256 
	// Do not double array size when expanding if it's bigger than 1/2 meg
	#define DOUBLING_BOUND 1024 * 512
	// Instead, expand by this amount every time we run out of space
	#define EXPAND_AMOUNT 1024 * 256
#else
	// Try not to allocate less than 1k
	#define MINIMAL_CHUNK 1024 
	// Do not double array size when expanding if it's bigger than 1 meg
	#define DOUBLING_BOUND 1024 * 1024
	// Instead, expand by this amount every time we run out of space
	#define EXPAND_AMOUNT 1024 * 512
#endif

// TODO: remove this
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef TRACK_MEMORY
DArray darray_create_tracked(size_t item_size, unsigned int reserve,
		const char* file, int line) {
	unsigned int initial_reserve = MINIMAL_CHUNK / item_size;
	if(initial_reserve == 0)
		initial_reserve = 1;

	reserve = MAX(reserve, initial_reserve);	
	DArray result = {NULL, item_size, 0, reserve};
	
	result.data = mem_alloc(item_size * reserve, file, line);

	return result;
}
#else
DArray darray_create_untracked(size_t item_size, unsigned int reserve) {
	unsigned int initial_reserve = MINIMAL_CHUNK / item_size;
	if(initial_reserve == 0)
		initial_reserve = 1;

	reserve = MAX(reserve, initial_reserve);	
	DArray result = {NULL, item_size, 0, reserve};
	
	result.data = MEM_ALLOC(item_size * reserve);

	return result;
}	
#endif

void darray_free(DArray* array) {
	assert(array);
	assert(array->data);
	
	if(array->reserved > 0) {
		MEM_FREE(array->data);
		array->data = NULL;
	}
	array->reserved = array->size = array->item_size = 0;
	assert(array->data == NULL);
}		

static void _expand_by_count(DArray* array, unsigned int count) {
	unsigned int expand_amount = EXPAND_AMOUNT / (array->item_size);
	if(expand_amount == 0)
		expand_amount = 1;

	unsigned int new_reservation = array->reserved * array->item_size > DOUBLING_BOUND ?	
		array->reserved + expand_amount : array->reserved * 2;

	if(new_reservation <= array->size + count)
		new_reservation = array->size + count;

	darray_reserve(array, new_reservation);	
}

void darray_append(DArray* array, const void* item_ptr) {
	assert(array);
	assert(array->data);
	assert(item_ptr);

	assert(array->size <= array->reserved);

	if(array->size == array->reserved) 
		_expand_by_count(array, 1);

	assert(array->size < array->reserved);

	memcpy(array->data + array->item_size * array->size, 
		item_ptr, array->item_size);
	array->size++;	
}

void darray_insert(DArray* array, unsigned int index, const void* item_ptr) {
	assert(array);
	assert(array->data);
	assert(index <= array->size);
	assert(item_ptr);
	assert(array->size <= array->reserved);

	if(index == array->size) {
		darray_append(array, item_ptr);
		return;
	}	

	if(array->size == array->reserved)
		_expand_by_count(array, 1);

	assert(array->size < array->reserved);	

	// Move all items starting from index forward
	void* addr = array->data + index * array->item_size;
	memmove(addr + array->item_size, addr, 
		array->item_size * (array->size - index));

	// Copy new item
	memcpy(addr, item_ptr, array->item_size);

	array->size++;
}

void darray_append_multi(DArray* array, const void* item_ptr, unsigned int count) {
	assert(array);
	assert(array->data);
	assert(item_ptr);
	assert(count);

	assert(array->size <= array->reserved);

	if(array->size + count > array->reserved) {
		// Not enough space, allocate more
		_expand_by_count(array, count);
	}	
	assert(array->size + count <= array->reserved);

	memcpy(array->data + array->item_size * array->size,
		item_ptr, array->item_size * count);
	array->size += count;	
}		

void darray_append_nulls(DArray* array, unsigned int count) {
	assert(array);
	assert(array->data);
	assert(count);
	assert(array->size <= array->reserved);

	if(array->size + count > array->reserved) {
		// Alloc space
		_expand_by_count(array, count);
	}
	assert(array->size + count <= array->reserved);

	memset(array->data + array->item_size * array->size, 0,
			array->item_size * count);
	array->size += count;
}

void darray_remove(DArray* array, unsigned int index) {
	assert(array);
	assert(array->data);
	assert(index < array->size);
	assert(array->size <= array->reserved);

	unsigned char* data = DARRAY_DATA_PTR(*array, unsigned char);
	unsigned int i = (index+1) * array->item_size;		

	for(; i < array->size * array->item_size; ++i) {
		data[i - array->item_size] = data[i];	
	}	
	array->size--;
}	

void darray_remove_fast(DArray* array, unsigned int index) {
	assert(array);
	assert(array->data);
	assert(index < array->size);
	assert(array->size <= array->reserved);

	memcpy(array->data + index * array->item_size, 
		array->data + (array->size-1) * array->item_size, array->item_size);
	array->size--;	
}	

void darray_reserve(DArray* array, unsigned int count) {
	assert(array);
	//assert(array->data);
	assert(array->size <= array->reserved);

	if(count <= array->reserved)
		return;
				
	array->data = MEM_REALLOC(array->data, count * array->item_size);	
	assert(array->data);

	array->reserved = count;
}	

void darray_shrink(DArray* array) {
	assert(array);
	assert(array->data);
	assert(array->size <= array->reserved);

	if(array->size == array->reserved)
		return;

	// Space for at least 1 item will be allocated
	unsigned int size = array->size == 0 ? 1 : array->size;
	array->data = MEM_REALLOC(array->data, size * array->item_size);
	assert(array->data);

	array->reserved = size;
}

void* darray_get(DArray* array, unsigned int i) {
    assert(array);
    assert(array->data);
    assert(array->size <= array->reserved);
    assert(i < array->size);
    
    return array->data + i * array->item_size;
}

