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
	node->next->prev = node->next;
	node->prev->next = node->prev;
}

