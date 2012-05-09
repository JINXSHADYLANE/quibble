#ifndef DARRAY_H
#define DARRAY_H

typedef struct {
	void* data;
	size_t item_size;
	unsigned int size;
	unsigned int reserved;
} DArray;	

// Creates new dynamic array.
#ifdef TRACK_MEMORY
DArray darray_create_tracked(size_t item_size, unsigned int reserve,
		const char* file, int line);
#define darray_create(item_size, reserve) darray_create_tracked(item_size, reserve, __FILE__, __LINE__)
#else
DArray darray_create_untracked(size_t item_size, unsigned int reserve);
#define darray_create(item_size, reserve) darray_create_untracked(item_size, reserve)
#endif

// Frees all memory used by array. 
void darray_free(DArray* array);

// Inserts new item to the end of array.
void darray_append(DArray* array, const void* item_ptr);
// Inserts multiple new items to the end of array
void darray_append_multi(DArray* array, const void* item_ptr, unsigned int count);
// Inserts multiple new items to the end of array, fills them with nulls
void darray_append_nulls(DArray* arra, unsigned int count);
// Inserts new item at specified index, shifts trailing items back
void darray_insert(DArray* array, unsigned int index, const void* item_ptr);
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

// Returns a pointer to i-th item.
void* darray_get(DArray* array, unsigned int i);

// Returns type-safe pointer to array items, eg.:
// int* int_array = DARRAY_DATA_PTR(int, darray);
#define DARRAY_DATA_PTR(darray, type) ((type*)(darray).data)

#endif

