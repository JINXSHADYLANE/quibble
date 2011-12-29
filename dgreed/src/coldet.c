#include "coldet.h"
#include "memory.h"

// Primes close to the powers of 2, for hashtable size 
static const uint primes[] = {
	61,
	127,
	251,
	509,
	1021,
	2039,
	4093,
	8191,
	16381,
	32749,
	65521,
	131071,
	262139,
	524287,
	1048573,
	2097143,
	4194301,
	8388593,
	16777213,
	33554393,
	67108859,
	134217689,
	268435399,
	536870909,
	1073741789
};

static uint _next_size(uint current_size) {
	// Find current size index with binary search
	uint idx = ~0;
	uint low = 0, high = ARRAY_SIZE(primes);
	while(high > low) {
		uint mid = (low + high) / 2;
		if(current_size < primes[mid])
			high = mid;
		else if(current_size > primes[mid])
			low = mid;
		else {
			idx = mid;
			break;
		}
	}
	assert(idx != ~0 && idx+1 < ARRAY_SIZE(primes));

	return primes[idx+1];
}

static uint _hash(int x, int y, int n) {
	int hash = 1178803047;
	hash += x * 422378729;
	hash += y * 2002318489;
	hash += n * 615241643;
	return hash;
}

static void _pt_cell(CDWorld* world, Vector2 pt, int* x, int* y) {
	assert(x && y);

	float x_int, y_int;
	modff(pt.x / world->cell_size, &x_int);
	modff(pt.y / world->cell_size, &y_int);
	*x = lrintf(x_int);
	*y = lrintf(y_int);
}

static void _coldet_hashmap_init(CDWorld* world, uint size) {
	assert(world);

	world->cells = MEM_ALLOC(sizeof(CDCell) * size);
	world->occupied_cells = 0;
	world->reserved_cells = size;

	// Init obj lists
	for(uint i = 0; i < size; ++i) {
		list_init(&world->cells[i].objs);
	}
}

static void _coldet_hashmap_close(CDWorld* world) {
	assert(world);

	MEM_FREE(world->cells);
}

static void _coldet_hashmap_insert_cell(CDWorld* world, CDCell* cell);

static void _coldet_hashmap_resize(CDWorld* world, bool force) {
	assert(world);

	if(force || world->occupied_cells >= world->reserved_cells * 3 / 4) {
		uint old_size = world->reserved_cells;
		uint new_size = _next_size(old_size);

		CDCell* old = world->cells; 
		_coldet_hashmap_init(world, new_size);

		// Rehash cells
		for(uint i = 0; i < old_size; ++i) {
			if(!list_empty(&old[i].objs)) {
				_coldet_hashmap_insert_cell(world, &old[i]);
			}
		}

		MEM_FREE(old);
	}
}

static bool _coldet_cell_insert(CDWorld* world, CDCell* cell, CDObj* obj, int x, int y) {
	if(list_empty(&cell->objs)) {
		// Occupy new cell
		world->occupied_cells++;
		cell->x = x;
		cell->y = y;
		list_push_back(&cell->objs, &obj->list);
		_coldet_hashmap_resize(world, false);
	}
	else if(cell->x == x && cell->y == y) {
		// Simply insert
		list_push_back(&cell->objs, &obj->list);
	}
	else
		return false;

	return true;
}

static bool _coldet_cell_move(CDWorld* world, CDCell* cell, CDCell* start) {
	if(cell == start)
		return false;

	assert(!list_empty(&cell->objs));

	uint hash1 = _hash(cell->x, cell->y, 0);
	uint hash2 = _hash(cell->x, cell->y, 1);

	uint n = world->reserved_cells;

	CDCell* alternative = &world->cells[hash1 % n] == cell ?
		&world->cells[hash2 % n] : &world->cells[hash1 % n];

	if(!list_empty(&alternative->objs)) {
		// Recursively move alternative cell
		if(!_coldet_cell_move(world, alternative, start))
			return false;
	}

	// Fix linked list pointers
	cell->objs.prev->next = &alternative->objs;
	cell->objs.next->prev = &alternative->objs;

	// Move cell
	*alternative = *cell;

	return true;
}

static void _coldet_hashmap_insert(CDWorld* world, CDObj* obj) {
	assert(world && obj);

	uint n;
	int x, y;
	_pt_cell(world, obj->pos, &x, &y);

try_again:

	n = world->reserved_cells;
	CDCell* cell = &world->cells[_hash(x, y, 0) % n];

	if(!_coldet_cell_insert(world, cell, obj, x, y)) {
		// Perform cuckoo swapping
		CDCell* start = cell;
		CDCell* alternative = &world->cells[_hash(x, y, 1) % n];

		if(!list_empty(&alternative->objs)) {
			if(!_coldet_cell_move(world, alternative, start)) {
				// Encountered cycle, resize table
				_coldet_hashmap_resize(world, true);
				goto try_again;
			}
		}

#ifdef _DEBUG
		assert(_coldet_cell_insert(world, alternative, obj, x, y));
#else
		_coldet_cell_insert(world, alternative, obj, x, y);
#endif
	}
}

static void _coldet_hashmap_insert_cell(CDWorld* world, CDCell* cell) {
	assert(world && cell);
	assert(!list_empty(&cell->objs));

	uint n;
	uint hash1 = _hash(cell->x, cell->y, 0);
	uint hash2 = _hash(cell->x, cell->y, 1);

try_again:

	n = world->reserved_cells;

	CDCell* dest = &world->cells[hash1 % n];
	if(list_empty(&dest->objs)) {
		// Fix linked list pointers
		cell->objs.prev->next = &dest->objs;
		cell->objs.next->prev = &dest->objs;

		// Dest is empty, overwrite
		*dest = *cell;
	}
	else {
		// Perform cuckoo swapping
		CDCell* start = dest;
		CDCell* alternative = &world->cells[hash2 % n];

		if(!list_empty(&alternative->objs)) {
			if(!_coldet_cell_move(world, alternative, start)) {
				// Cycle, must resize table
				LOG_WARNING("_coldet_hashmap_insert_cell triggered table resize!");
				_coldet_hashmap_resize(world, true);
				goto try_again;
			}
		}

		assert(list_empty(&alternative->objs));

		// Fix linked list pointers
		cell->objs.prev->next = &alternative->objs;
		cell->objs.next->prev = &alternative->objs;

		*alternative = *cell;
	}
}

static void _coldet_hashmap_remove(CDWorld* world, CDObj* obj) {
	int x, y;
	_pt_cell(world, obj->pos, &x, &y);

	// Find correct cell. Cuckoo hashing assures it will be one
	// of two alternatives.
	uint n = world->reserved_cells;
	CDCell* cell = &world->cells[_hash(x, y, 0) % n];
	if(cell->x != x || cell->y != y)
		cell = &world->cells[_hash(x, y, 1) % n];

	assert(cell->x == x && cell->y == y);
}

