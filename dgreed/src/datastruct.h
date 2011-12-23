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
int heap_size(Heap* heap);
void heap_push(Heap* heap, int weight, void* data);
int heap_peek(Heap* heap, void** data);
int heap_pop(Heap* heap, void** data);

#endif
