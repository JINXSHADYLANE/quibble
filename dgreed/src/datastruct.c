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


// AA tree

typedef uint AATNodeIdx;

#define INVALID_IDX ~0

typedef struct AATNode {
	int key;
	void* data;
	int level;
	AATNodeIdx left;
	AATNodeIdx right;
	AATNodeIdx parent; // This is needed for relabeling nodes
} AATNode;

static AATNode* _aatree_get_node(DArray* tree, AATNodeIdx idx) {
	assert(tree);
	assert(tree->item_size == sizeof(AATNode));
	assert(idx < tree->size);

	AATNode* nodes = DARRAY_DATA_PTR(*tree, AATNode);
	return &nodes[idx];
}

static AATNodeIdx _aatree_new_node(DArray* tree, AATNode* node) {
	assert(tree);
	assert(tree->item_size == sizeof(AATNode));

	AATNodeIdx result = tree->size;

	darray_append(tree, node);

	return result;
}

static void _aatree_mark_delete(AATree* tree, AATNodeIdx idx) {
	tree->del_idx = idx;
#if _DEBUG
	AATNode* nodes = DARRAY_DATA_PTR(tree->tree, AATNode);
	nodes[idx].left = nodes[idx].right = nodes[idx].parent = INVALID_IDX;
	nodes[idx].level = nodes[idx].key = 0;
	nodes[idx].data = NULL;
#endif
}

static void _aatree_delete_node(AATree* tree) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));
	assert(tree->del_idx < tree->tree.size);

	AATNode* nodes = DARRAY_DATA_PTR(tree->tree, AATNode);
	AATNodeIdx idx = tree->del_idx;
	AATNodeIdx last = tree->tree.size-1;

	// Copy last node to its place
	nodes[idx] = nodes[last];	

	// Relabel node we just moved
	if(nodes[idx].parent != INVALID_IDX) {
		AATNode* parent = &nodes[nodes[idx].parent];
		if(parent->left == last)
			parent->left = idx;
		else if(parent->right == last)
			parent->right = idx;
		else
			assert(0 && "Something went horribly wrong.");
	}

	// Relabel root is neccessary
	if(tree->root == last)
		tree->root = idx;

	// Decrease node count
	tree->tree.size--;
}

static AATNodeIdx _aatree_skew(DArray* tree, AATNodeIdx idx) {
	assert(tree);
	assert(tree->item_size == sizeof(AATNode));
	assert(idx < tree->size);

	AATNode* node = _aatree_get_node(tree, idx);
	AATNode* node_left = NULL;
	AATNode* node_left_right = NULL;
	if(node->left != INVALID_IDX) {
		node_left = _aatree_get_node(tree, node->left);
		if(node_left->right != INVALID_IDX) {
			node_left_right = _aatree_get_node(tree, node_left->right);
		}
	}

	if(node_left && node->level == node_left->level) {
		// Rotate right
		AATNodeIdx temp = idx;
		idx = node->left;
		node->left = node_left->right;
		node_left->right = temp;
		node->parent = idx;
		node_left->parent = INVALID_IDX;
		if(node_left_right)
			node_left_right->parent = temp; 
	}

	return idx;
}

static AATNodeIdx _aatree_split(DArray* tree, AATNodeIdx idx) {
	assert(tree);
	assert(tree->item_size == sizeof(AATNode));
	assert(idx < tree->size);

	AATNode* node = _aatree_get_node(tree, idx);
	AATNode* node_right = NULL;
	AATNode* node_right_right = NULL;
	AATNode* node_right_left = NULL;

	if(node->right != INVALID_IDX) {
		node_right = _aatree_get_node(tree, node->right);
		if(node_right->right != INVALID_IDX)
			node_right_right = _aatree_get_node(tree, node_right->right);
		if(node_right->left != INVALID_IDX)
			node_right_left = _aatree_get_node(tree, node_right->left);
	}

	if(node_right_right && node->level == node_right_right->level) {
		// Rotate left
		AATNodeIdx temp = idx;
		idx = node->right;
		node->right = node_right->left;
		node_right->left = temp;
		node->parent = idx;
		node_right->level++;
		node_right->parent = INVALID_IDX;
		if(node_right_left)
			node_right_left->parent = temp;
	}

	return idx;
}

void aatree_init(AATree* tree) {
	assert(tree);

	tree->root = INVALID_IDX;
	tree->tree = darray_create(sizeof(AATNode), 0);
}

void aatree_free(AATree* tree) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));

	darray_free(&tree->tree);
	tree->root = INVALID_IDX;
}

uint aatree_size(AATree* tree) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));

	return tree->tree.size;
}

void aatree_clear(AATree* tree) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));

	tree->tree.size = 0;
	tree->root = INVALID_IDX;
}

void* aatree_find(AATree* tree, int key) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));

	AATNodeIdx idx = tree->root;	
	
	while(1) {
		if(idx == INVALID_IDX)
			return NULL; // Not found

		AATNode* node = _aatree_get_node(&tree->tree, idx);

		if(key < node->key)
			idx = node->left;
		else if(key > node->key)
			idx = node->right;
		else
			return node->data;
	}
}

int aatree_min(AATree* tree, void** data) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));
	assert(tree->tree.size);

	AATNodeIdx idx = tree->root;

	while(1) {
		AATNode* node = _aatree_get_node(&tree->tree, idx);

		if(node->left == INVALID_IDX) {
			if(data)
				*data = node->data;
			return node->key;
		}
		else {
			idx = node->left;
		}
	}
}

int aatree_max(AATree* tree, void** data) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));
	assert(tree->tree.size);

	AATNodeIdx idx = tree->root;

	while(1) {
		AATNode* node = _aatree_get_node(&tree->tree, idx);

		if(node->right == INVALID_IDX) {
			if(data)
				*data = node->data;
			return node->key;
		}
		else {
			idx = node->right;
		}
	}
}

static AATNodeIdx _aatree_insert(DArray* tree, AATNodeIdx idx, int key, void* data) {

	if(idx == INVALID_IDX) {
		// We're at the bottom of the tree, make new node
		AATNode new = {
			.key = key,
			.data = data,
			.level = 1,
			.left = INVALID_IDX,
			.right = INVALID_IDX,
			.parent = INVALID_IDX // Parent will fill this
		};

		return _aatree_new_node(tree, &new);
	}

	AATNode* node = _aatree_get_node(tree, idx);

	if(key < node->key) {
		// Recursively insert to left subtree
		AATNodeIdx new_idx = _aatree_insert(tree, node->left, key, data);
		if(new_idx == INVALID_IDX)
			return INVALID_IDX;

		// Update left pointer and children parent pointer
		node = _aatree_get_node(tree, idx);
		node->left = new_idx;
		AATNode* new = _aatree_get_node(tree, new_idx);
		new->parent = idx;
	}
	else if(key > node->key) {
		// Recursively insert to right subtree
		AATNodeIdx new_idx = _aatree_insert(tree, node->right, key, data);
		if(new_idx == INVALID_IDX)
			return INVALID_IDX;

		// Update right pointer and children parent pointer
		node = _aatree_get_node(tree, idx);
		node->right = new_idx;
		AATNode* new = _aatree_get_node(tree, new_idx);
		new->parent = idx;
	}
	else {
		// Key is not unique!
		return INVALID_IDX;
	}

	// Rebalance on the way back
	idx = _aatree_skew(tree, idx);
	idx = _aatree_split(tree, idx);

	return idx;
}

bool aatree_insert(AATree* tree, int key, void* data) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));

	AATNodeIdx new_root = _aatree_insert(&tree->tree, tree->root, key, data);

	if(new_root == INVALID_IDX)
		return false;

	tree->root = new_root;
	return true;
}

static AATNodeIdx _aatree_remove(AATree* tree, AATNodeIdx idx, int key) {
	if(idx == INVALID_IDX)
		// Bottom reached!
		return INVALID_IDX;

	tree->rem_last = idx;
	
	AATNode* node = _aatree_get_node(&tree->tree, idx);
	if(key < node->key) {
		node->left = _aatree_remove(tree, node->left, key);
		if(node->left != INVALID_IDX) {
			AATNode* new_child = _aatree_get_node(&tree->tree, node->left);
			new_child->parent = idx;
		}
	}
	else {
		tree->rem_candidate = idx;
		node->right = _aatree_remove(tree, node->right, key);
		if(node->right != INVALID_IDX) {
			AATNode* new_child = _aatree_get_node(&tree->tree, node->right);
			new_child->parent = idx;
		}
	}

	// Get pointer to delete candidate node
	AATNode* candidate_node = NULL;
	if(tree->rem_candidate != INVALID_IDX)
		candidate_node = _aatree_get_node(&tree->tree, tree->rem_candidate);

	// Get pointers to current, left and right nodes
	node = _aatree_get_node(&tree->tree, idx);
	AATNode* node_left = NULL;
	if(node->left != INVALID_IDX)
		node_left = _aatree_get_node(&tree->tree, node->left);
	AATNode* node_right = NULL;
	if(node->right != INVALID_IDX)
		node_right = _aatree_get_node(&tree->tree, node->right);

	if(idx == tree->rem_last && candidate_node && candidate_node->key == key) {
		// Remove entry by moving last one to its place	
		candidate_node->key = node->key;
		tree->rem_data = candidate_node->data;
		candidate_node->data = node->data;
		tree->rem_candidate = INVALID_IDX;
		idx = node->right;
		
		_aatree_mark_delete(tree, tree->rem_last);
		tree->did_del = true;
	}
	else if ((node_left && node->level-1 > node_left->level) ||
			 (node_right && node->level-1 > node_right->level)) {

		// Perform rebalancing choreography on the way back
		assert(tree->did_del);
		
		node->level--;
		if(node_right && node->level < node_right->level)
			node_right->level = node->level;

		idx = _aatree_skew(&tree->tree, idx);
		node = _aatree_get_node(&tree->tree, idx);
		node_right = NULL;

		if(node->right != INVALID_IDX) {
			node->right = _aatree_skew(&tree->tree, node->right);
			node_right = _aatree_get_node(&tree->tree, node->right);
		}

		if(node_right && node_right->right != INVALID_IDX) {
			node_right->right = _aatree_skew(&tree->tree, node_right->right);
		}

		idx = _aatree_split(&tree->tree, idx);
		node = _aatree_get_node(&tree->tree, idx);

		if(node->right != INVALID_IDX) 
			node->right = _aatree_split(&tree->tree, node->right);
	}

	return idx;
}

void* aatree_remove(AATree* tree, int key) {
	assert(tree);
	assert(tree->tree.item_size == sizeof(AATNode));
	assert(tree->tree.size);
	assert(tree->root != INVALID_IDX);

	// Prep 'global' state
	tree->rem_last = tree->rem_candidate = INVALID_IDX;
	tree->rem_data = NULL;
	tree->did_del = false;

	tree->root = _aatree_remove(tree, tree->root, key);
	if(tree->did_del)
		_aatree_delete_node(tree);
	assert(tree->root != INVALID_IDX || tree->tree.size == 0);

	return tree->rem_data;
}

