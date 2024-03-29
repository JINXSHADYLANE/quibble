#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "darray.h"
#include "memory.h"

#define MINIMAL_CHUNK 256 
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef TRACK_MEMORY
DArray darray_create_tracked(
		size_t item_size, unsigned int reserve,
		const char* file, int line
	) {
	
	if(!reserve) 
		reserve = MAX(1, MINIMAL_CHUNK / item_size);
	
	DArray darr = {
		.data = mem_alloc(item_size * reserve, file, line),
		.item_size = item_size,
		.size = 0,
		.reserved = reserve
	};

	return darr;
}
#else
DArray darray_create_untracked(
		size_t item_size, unsigned int reserve
	) {
	
	if(!reserve) 
		reserve = MAX(1, MINIMAL_CHUNK / item_size);
	
	DArray darr = {
		.data = MEM_ALLOC(item_size * reserve),
		.item_size = item_size,
		.size = 0,
		.reserved = reserve
	};

	return darr;
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

void darray_append(DArray* array, const void* item_ptr) {
	assert(array);
	assert(array->data);
	assert(item_ptr);
	assert(array->size <= array->reserved);

	size_t s = array->size;

	if(s == array->reserved) 
		darray_reserve(array, s * 2);
	assert(s < array->reserved);

	memcpy(
		array->data + array->item_size * s, 
		item_ptr, 
		array->item_size
	);

	array->size++;	
}

void darray_insert(DArray* array, unsigned int index, const void* item_ptr) {
	assert(array);
	assert(array->data);
	assert(index <= array->size);
	assert(item_ptr);
	assert(array->size <= array->reserved);

	size_t s = array->size;
	
	if(array->size == array->reserved)
		darray_reserve(array, s * 2);
	assert(s < array->reserved);	

	// Move all items starting from index forward
	void* addr = array->data + index * array->item_size;
	memmove(
		addr + array->item_size, 
		addr, 
		array->item_size * (array->size - index)
	);

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

	size_t s = array->size;
	size_t r = array->reserved;

	while(s + count > r)
		r *= 2;
	if(r > array->reserved)
		darray_reserve(array, r);
	assert(s + count <= array->reserved);

	memcpy(
		array->data + array->item_size * s,
		item_ptr,
		array->item_size * count
	);
	array->size += count;	
}		

void darray_append_nulls(DArray* array, unsigned int count) {
	assert(array);
	assert(array->data);
	assert(count);
	assert(array->size <= array->reserved);

	size_t s = array->size;
	size_t r = array->reserved;

	while(s + count > r)
		r *= 2;
	if(r > array->reserved)
		darray_reserve(array, r);
	assert(s + count <= array->reserved);

	memset(
		array->data + array->item_size * s,
		0, array->item_size * count
	);
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

