#ifndef DATASTRUCT_H
#define DATASTRUCT_H

#include "utils.h"
#include "darray.h"

// Kernel style circular doubly linked list

typedef struct ListHead {
	struct ListHead* next;
	struct ListHead* prev; 
} ListHead;

void list_init(ListHead* head);
bool list_empty(ListHead* head);
void list_push_back(ListHead* head, ListHead* new);
void list_push_front(ListHead* head, ListHead* new);
void list_insert_after(ListHead* node, ListHead* new);
ListHead* list_pop_back(ListHead* head);
ListHead* list_pop_front(ListHead* head);
void list_remove(ListHead* node);

#define list_entry(ptr, type, member) \
	((type*)((char*)(ptr)-(size_t)(&((type*)0)->member)))	

#define list_first_entry(head, type, member) \
	list_entry((head)->next, type, member)

#define list_last_entry(head, type, member) \
	list_entry((head)->prev, type, member)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, __typeof__(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, __typeof__(*pos), member))


// Binary min-heap priority queue

typedef DArray Heap;

void heap_init(Heap* heap);
void heap_free(Heap* heap);
int heap_size(Heap* heap);							// O(1)
void heap_push(Heap* heap, int weight, void* data); // O(n log n)
int heap_peek(Heap* heap, void** data);				// O(1)
int heap_pop(Heap* heap, void** data);				// O(n log n)


// AA tree based integer set/map

typedef struct {
	uint root, rem_last, rem_candidate;
	void* rem_data;
	uint del_idx;
	bool did_del;
	DArray tree;
} AATree;

void aatree_init(AATree* tree);
void aatree_free(AATree* tree);
uint aatree_size(AATree* tree);							// O(1)
void aatree_clear(AATree* tree);						// O(1)
void* aatree_find(AATree* tree, int key);				// O(n log n)
int aatree_min(AATree* tree, void** data);				// O(n log n)
int aatree_max(AATree* tree, void** data);				// O(n log n)
bool aatree_insert(AATree* tree, int key, void* data);  // O(n log n)
void* aatree_remove(AATree* tree, int key);				// O(n log n)

#endif
