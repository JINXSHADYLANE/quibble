#ifndef DARRAY_H
#define DARRAY_H

typedef struct {
	void* data;
	size_t item_size;
	unsigned int size;
	unsigned int reserved;
} DArray;	

// Creates new dynamic array.
DArray darray_create(size_t item_size, unsigned int reserve);
// Frees all memory used by array. 
void darray_free(DArray* array);

// Inserts new item to the end of array.
void darray_append(DArray* array, void* item_ptr);
// Inserts multiple new items to the end of array
void darray_append_multi(DArray* array, void* item_ptr, unsigned int count);
// Removes item and preserves array order.
void darray_remove(DArray* array, unsigned int index);
// Quickly removes item by copying last item to its place.
void darray_remove_fast(DArray* array, unsigned int index);

// Ensures that darray can hold at least the provided number of items.
// This is costly operation, don't call too often.
// Size is not updated, only reserved size!
void darray_reserve(DArray* array, unsigned int count);

// Shrinks memory used by array, to hold only the items it currently has.
// Useful for reducing memory usage when you know array will not be expanding.
void darray_shrink(DArray* array);

// Returns type-safe pointer to array items, eg.:
// int* int_array = DARRAY_DATA_PTR(int, darray);
#define DARRAY_DATA_PTR(darray, type) ((type*)(darray).data)

#endif

