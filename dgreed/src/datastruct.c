#include "datastruct.h"

// List

void list_init(ListHead* head) {
	head->next = head;
	head->prev = head;
}

bool list_empty(ListHead* head) {
	return head->next == head;
}

void list_push_back(ListHead* head, ListHead* new) {
	ListHead* last = head->prev;
	last->next = new;
	head->prev = new;
	new->next = head;
	new->prev = last;
}

void list_push_front(ListHead* head, ListHead* new) {
	ListHead* first = head->next;
	first->prev = new;
	head->next = new;
	new->next = first;
	new->prev = head;
}

void list_insert_after(ListHead* node, ListHead* new) {
	node->next->prev = new;
	new->next = node->next;
	node->next = new;
	new->prev = node;
}

ListHead* list_pop_back(ListHead* head) {
	ListHead* last = head->prev;
	list_remove(last);

#ifdef _DEBUG
	last->prev = NULL;
	last->next = NULL;
#endif

	return last;
}

ListHead* list_pop_front(ListHead* head) {
	ListHead* first = head->next;
	list_remove(first);

#ifdef _DEBUG
	first->prev = NULL;
	first->next = NULL;
#endif

	return first;
}

void list_remove(ListHead* node) {
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

// Minheap

typedef struct {
	int weight;
	void* data;
} HeapNode;

static uint _heap_parent(uint i) {
	assert(i != 0);
	return (i-1) / 2;
}

static uint _heap_first_child(uint i) {
	return i*2 + 1;
}

static uint _heap_smaller_child(Heap* heap, uint i) {
	HeapNode* nodes = DARRAY_DATA_PTR(*heap, HeapNode);

	uint ca = _heap_first_child(i);
	uint cb = ca + 1;
	if(ca >= heap->size)
		return 0;

	if(cb >= heap->size)
		return ca;

	if(nodes[ca].weight > nodes[cb].weight)
		return cb;
	else
		return ca;
}

void heap_init(Heap* heap) {
	*heap = darray_create(sizeof(HeapNode), 0);
}

void heap_free(Heap* heap) {
	assert(heap);
	assert(heap->item_size == sizeof(HeapNode));
	darray_free(heap);
}

int heap_size(Heap* heap) {
	assert(heap);
	assert(heap->item_size == sizeof(HeapNode));
	return heap->size;
}

void heap_push(Heap* heap, int weight, void* data) {
	assert(heap);
	assert(heap->item_size == sizeof(HeapNode));

	HeapNode new = {weight, data};

	darray_append(heap, &new);

	HeapNode* nodes = DARRAY_DATA_PTR(*heap, HeapNode);
	uint i = heap->size - 1;			// New node index
	uint ip = i ? _heap_parent(i) : 0;  // New node parent index

	// Swap newly inserted node with parent until heap property is satisfied
	while(i && nodes[i].weight < nodes[ip].weight) {
		HeapNode temp = nodes[i];
		nodes[i] = nodes[ip];
		nodes[ip] = temp;

		i = ip;
		ip = i ? _heap_parent(i) : 0;
	}
}

int heap_peek(Heap* heap, void** data) {
	assert(heap);
	assert(heap->item_size == sizeof(HeapNode));
	assert(heap->size);

	HeapNode* nodes = DARRAY_DATA_PTR(*heap, HeapNode);
	if(data)
		*data = nodes[0].data;

	return nodes[0].weight;
}

int heap_pop(Heap* heap, void** data) {
	assert(heap);
	assert(heap->item_size == sizeof(HeapNode));
	assert(heap->size);

	HeapNode* nodes = DARRAY_DATA_PTR(*heap, HeapNode);
	
	// Remember min node
	HeapNode min = nodes[0];

	// Copy last node into beginning, decrease size
	darray_remove_fast(heap, 0);
	uint i = 0;
	uint ic = _heap_smaller_child(heap, i);

	// Swap first node with smallest child until heap property is satisfied
	while(ic && nodes[ic].weight < nodes[i].weight) {
		HeapNode temp = nodes[i];
		nodes[i] = nodes[ic];
		nodes[ic] = temp;

		i = ic;
		ic = _heap_smaller_child(heap, i);
	}

	if(data)
		*data = min.data;

	return min.weight;
}

