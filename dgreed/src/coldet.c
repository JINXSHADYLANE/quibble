#include "coldet.h"
#include "memory.h"

#define HORIZ_WRAP(x, x_off) {\
	int horiz_cells = lrintf(world->width / world->cell_size); \
	x_off = world->width * (float)(x / horiz_cells); \
	if(x < 0 && x % horiz_cells != 0) x_off -= world->width; \
	x = ((x % horiz_cells) + horiz_cells) % horiz_cells; \
}

#define VERT_WRAP(y, y_off) {\
	int vert_cells = lrintf(world->height / world->cell_size); \
	y_off = world->height * (float)(y / vert_cells); \
	if(y < 0 && y % vert_cells != 0) y_off -= world->height; \
	y = ((y % vert_cells) + vert_cells) % vert_cells; \
}

static const uint max_swapchain_depth = 10;

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
	uint hash = 1178803047;
	hash += x * 422378729;
	hash ^= (hash << 17) | (hash >> 16);
	hash += y * 2002318489;
	hash ^= (hash << 17) | (hash >> 16);
	hash += n * 615241643;
	hash ^= (hash << 17) | (hash >> 16);
	return hash;
}

static void _pt_cell(CDWorld* world, Vector2 pt, int* x, int* y) {
	assert(x && y);

	float x_int, y_int;
	modff(pt.x / world->cell_size, &x_int);
	modff(pt.y / world->cell_size, &y_int);
	*x = lrintf(x_int);
	*y = lrintf(y_int);

	if(pt.x < 0.0f)
		*x -= 1;
	if(pt.y < 0.0f)
		*y -= 1;
}

static void _obj_cell(CDWorld* world, CDObj* obj, int* x, int* y) {
	Vector2 pt = obj->pos;			// Upper left corner
	if(obj->type == CD_CIRCLE)
		pt = vec2_sub(pt, vec2(obj->size.radius, obj->size.radius));

	_pt_cell(world, pt, x, y);

	if(world->horiz_wrap) {
		float x_off;
		HORIZ_WRAP(*x, x_off);
		obj->pos.x -= x_off;
	}

	if(world->vert_wrap) {
		float y_off;
		VERT_WRAP(*y, y_off);
		obj->pos.y -= y_off;
	}
}

static RectF _rectf_from_cell(CDWorld* world, int x, int y) {
	assert(world);

	RectF rect = {
		.left = (float)x * world->cell_size,
		.top = (float)y * world->cell_size,
		.right = ((float)x + 1.0f) * world->cell_size,
		.bottom = ((float)y + 1.0f) * world->cell_size
	};

	return rect;
}

static RectF _rectf_from_aabb(CDObj* obj) {
	assert(obj);
	assert(obj->type == CD_AABB);

	RectF rect = {
		.left = obj->pos.x,
		.top = obj->pos.y,
		.right = obj->pos.x + obj->size.size.x,
		.bottom = obj->pos.y + obj->size.size.y
	};

	return rect;
}

static void _coldet_hashmap_init(CDWorld* world, uint size) {
	assert(world);

	world->cells = MEM_ALLOC(sizeof(CDCell) * size);

	// Set cell x and y so that we won't be reading uninitialized values in
	// future
	for(uint i = 0; i < size; ++i) {
		world->cells[i].x = MAX_INT32;
		world->cells[i].y = MAX_INT32;
	}

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
		uint occupied = world->occupied_cells;
		uint old_size = world->reserved_cells;
		uint new_size = _next_size(old_size);

		CDCell* old = world->cells; 
		_coldet_hashmap_init(world, new_size);

		world->occupied_cells = occupied;

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
	assert(world && cell && obj);

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

static bool _coldet_cell_move(CDWorld* world, CDCell* cell, CDCell* start,
		uint depth) {
	if(cell == start)
		return false;
	if(depth > max_swapchain_depth)
		return false;

	assert(!list_empty(&cell->objs));

	uint hash1 = _hash(cell->x, cell->y, 0);
	uint hash2 = _hash(cell->x, cell->y, 1);

	uint n = world->reserved_cells;

	CDCell* alternative = &world->cells[hash1 % n] == cell ?
		&world->cells[hash2 % n] : &world->cells[hash1 % n];

	if(!list_empty(&alternative->objs)) {
		// Recursively move alternative cell
		if(!_coldet_cell_move(world, alternative, start, depth+1))
			return false;
	}

	// Fix linked list pointers
	cell->objs.prev->next = &alternative->objs;
	cell->objs.next->prev = &alternative->objs;

	// Move cell
	*alternative = *cell;

	// Make cell empty
	list_init(&cell->objs);	

	return true;
}

static void _coldet_hashmap_insert(CDWorld* world, CDObj* obj) {
	assert(world && obj);

	uint n;
	int x, y;
	_obj_cell(world, obj, &x, &y);

try_again:

	n = world->reserved_cells;

	CDCell* cell_a = &world->cells[_hash(x, y, 0) % n];
	CDCell* cell_b = &world->cells[_hash(x, y, 1) % n];

	CDCell* cell;

	if(cell_b->x == x && cell_b->y == y)
		cell = cell_b;
	else
		cell = cell_a;

	if(!_coldet_cell_insert(world, cell, obj, x, y)) {
		CDCell* start = cell;

		CDCell* alternative = (cell == cell_a) ? cell_b : cell_a;

		// Perform cuckoo swapping
		if(!list_empty(&alternative->objs)) {
			if(!_coldet_cell_move(world, alternative, start, 0)) {
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

	int x = cell->x;
	int y = cell->y;
	uint n;

try_again:

	n = world->reserved_cells;

	CDCell* dest = &world->cells[_hash(x, y, 0) % n];
	if(!list_empty(&dest->objs))
		dest = &world->cells[_hash(x, y, 1) % n];

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
		CDCell* alternative = (dest == &world->cells[_hash(x, y, 0) % n]) ?
			&world->cells[_hash(x, y, 1) % n] : &world->cells[_hash(x, y, 0) % n];

		if(!list_empty(&alternative->objs)) {
			if(!_coldet_cell_move(world, alternative, start, 0)) {
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
	ListHead* next = obj->list.next;

	list_remove(&obj->list);	

	if(list_empty(next))
		world->occupied_cells--;
}

static CDCell* _coldet_hashmap_get(CDWorld* world, int x, int y,
		float* x_offset, float* y_offset) {
	uint n = world->reserved_cells;

	if(world->horiz_wrap) 
		HORIZ_WRAP(x, *x_offset);

	if(world->vert_wrap) 
		VERT_WRAP(y, *y_offset);

	// Find correct cell. Cuckoo hashing assures it will be one
	// of two alternatives.
	CDCell* cell = &world->cells[_hash(x, y, 0) % n];
	if(cell->x != x || cell->y != y)
		cell = &world->cells[_hash(x, y, 1) % n];

	if(cell->x != x || cell->y != y)
		return NULL;
	
	return list_empty(&cell->objs) ? NULL : cell;
}

// Public interface

void coldet_init(CDWorld* cd, float max_obj_size) {
	coldet_init_ex(cd, max_obj_size, 0.0f, 0.0f, false, false);
}

void coldet_init_ex(CDWorld* cd, float max_obj_size,
		float width, float height, bool horiz_wrap, bool vert_wrap) {
	assert(cd);
	assert(max_obj_size > 0.01f && max_obj_size < 10000.0f);

	float int_part, cell_size = max_obj_size;
	if(horiz_wrap) {
		assert(width >= cell_size*2.0f);
		// Increase cell size to fit integral number of cells vertically
		if(fabsf(modff(width / cell_size, &int_part)) > 0.001f) {
			cell_size = width / (int_part-1.0f);
		}
	}

	if(vert_wrap) {
		assert(height >= cell_size*2.0f);
		// Increase cell size to fit integral number of cells horizontally 
		if(fabsf(modff(height / cell_size, &int_part)) > 0.001f) {
			cell_size = height / (int_part-1.0f);
		}
	}

	assert(fabsf(modff(width / cell_size, &int_part)) <= 0.1f);
	assert(fabsf(modff(height / cell_size, &int_part)) <= 0.1f);

	cd->cell_size = cell_size;
	cd->horiz_wrap = horiz_wrap;
	cd->vert_wrap = vert_wrap;
	cd->width = width;
	cd->height = height;

	mempool_init(&cd->allocator, sizeof(CDObj));
	list_init(&cd->reinsert_list);

#ifndef NO_DEVMODE
	cd->last_process_hittests = 0;
	cd->last_process_reinserts = 0;
	cd->max_objs_in_cell = 0;
#endif

	_coldet_hashmap_init(cd, primes[0]);
}

void coldet_close(CDWorld* cd) {
	assert(cd);

	_coldet_hashmap_close(cd);

	mempool_drain(&cd->allocator);
}

CDObj* coldet_new_circle(CDWorld* cd, Vector2 center, float radius,
		uint mask, void* userdata) {
	assert(cd);
	assert(mask); // If mask is 0, object will be just ghost
	assert(radius > 0.0f && radius*2.0f <= cd->cell_size);

	CDObj* new = mempool_alloc(&cd->allocator);
	
	new->pos = center;
	new->size.radius = radius;
	
	new->offset = vec2(0.0f, 0.0f);
	new->dirty = false;
	new->type = CD_CIRCLE;
	new->mask = mask;
	new->userdata = userdata;

	_coldet_hashmap_insert(cd, new); 

	return new;
}

CDObj* coldet_new_aabb(CDWorld* cd, const RectF* rect, uint mask,
		void* userdata) {
	assert(cd);
	assert(mask); // If mask is 0, object will be just ghost

	CDObj* new = mempool_alloc(&cd->allocator);
	
	new->pos = vec2(rect->left, rect->top);
	new->size.size = vec2(rectf_width(rect), rectf_height(rect));

	assert(new->size.size.x <= cd->cell_size);
	assert(new->size.size.y <= cd->cell_size);
	
	new->offset = vec2(0.0f, 0.0f);
	new->dirty = false;
	new->type = CD_AABB;
	new->mask = mask;
	new->userdata = userdata;

	_coldet_hashmap_insert(cd, new); 

	return new;
}

void coldet_remove_obj(CDWorld* cd, CDObj* obj) {
	assert(cd);
	
	_coldet_hashmap_remove(cd, obj);
	mempool_free(&cd->allocator, obj);
}

uint coldet_query_circle(CDWorld* cd, Vector2 center, float radius, uint mask,
		CDQueryCallback callback) {
	assert(cd);
	assert(mask);

	int count = 0;

	int ul_x, ul_y; // Upper left cell
	int lr_x, lr_y; // Lower right cell

	_pt_cell(cd, vec2(center.x - radius, center.y - radius), &ul_x, &ul_y);
	_pt_cell(cd, vec2(center.x + radius, center.y + radius), &lr_x, &lr_y);

	assert(ul_x <= lr_x);
	assert(ul_y <= lr_y);

	// If circle is big enough, we can sometimes skip individual collission checks
	bool can_skip_tests = (radius - cd->cell_size) > cd->cell_size * 2.0f;

	for(int y = ul_y-1; y <= lr_y; ++y) {
		for(int x = ul_x-1; x <= lr_x; ++x) {
			float x_off = 0.0f, y_off = 0.0f;

			// Iterate through every colliding cell
			CDCell* cell = _coldet_hashmap_get(cd, x, y, &x_off, &y_off);
			if(cell == NULL)
				continue;

			// If cell rect is completely inside circle,
			// don't bother with individual collission checks
			if(can_skip_tests) {
				RectF cell_rect = _rectf_from_cell(cd, x, y);
				if(rectf_inside_circle(&cell_rect, &center, radius - cd->cell_size)) {
					CDObj* pos;
					list_for_each_entry(pos, &cell->objs, list) {
						if(pos->mask & mask) {
							if(callback)
								(*callback)(pos);
							count++;
						}
					}
					continue;
				}
			}

			// Perform full collission checks
			CDObj* pos;
			list_for_each_entry(pos, &cell->objs, list) {
				if(pos->mask & mask) {
					// AABB
					if(pos->type == CD_AABB) {
						RectF aabb = _rectf_from_aabb(pos);
						aabb.left += x_off; aabb.right += x_off;
						aabb.top += y_off; aabb.bottom += y_off;
						if(rectf_circle_collision(&aabb, &center, radius)) {
							if(callback)
								(*callback)(pos);
							count++;
						}
					}
					// Circle
					if(pos->type == CD_CIRCLE) {
						Vector2 p = vec2(pos->pos.x+x_off, pos->pos.y+y_off);
						Vector2 d = vec2_sub(center, p);
						float rr = radius + pos->size.radius;
						if(vec2_length_sq(d) <= rr * rr) {
							if(callback)
								(*callback)(pos);
							count++;
						}
					}
				}
			}
		}
	}

	return count;
}

uint coldet_query_aabb(CDWorld* cd, const RectF* rect, uint mask,
		CDQueryCallback callback) {
	assert(cd);
	assert(mask);

	int count = 0;
	int ul_x, ul_y; // Upper left cell
	int lr_x, lr_y; // Lower right cell

	_pt_cell(cd, vec2(rect->left, rect->top), &ul_x, &ul_y);
	_pt_cell(cd, vec2(rect->right, rect->bottom), &lr_x, &lr_y);

	assert(ul_x <= lr_x);
	assert(ul_y <= lr_y);

	for(int y = ul_y-1; y <= lr_y; ++y) {
		for(int x = ul_x-1; x <= lr_x; ++x) {
			float x_off = 0.0f, y_off = 0.0f;

			// Iterate through every colliding cell
			CDCell* cell = _coldet_hashmap_get(cd, x, y, &x_off, &y_off);
			if(cell == NULL)
				continue;

			// Don't check for collissions with individual objects if
			// we're sure cell is completely inside AABB
			if(ul_y < y && y < lr_y && ul_x < x && x < lr_x) {
				CDObj* pos;
				list_for_each_entry(pos, &cell->objs, list) {
					if(pos->mask & mask) {
						if(callback)
							(*callback)(pos);
						count++;
					}
				}
				continue;
			}

			// Perform collission checks for boundary objects
			CDObj* pos;
			list_for_each_entry(pos, &cell->objs, list) {
				if(pos->mask & mask) {
					// AABB
					if(pos->type == CD_AABB) {
						RectF aabb = _rectf_from_aabb(pos);
						aabb.left += x_off; aabb.right += x_off;
						aabb.top += y_off; aabb.bottom += y_off;
						if(rectf_rectf_collision(rect, &aabb)) {
							if(callback)
								(*callback)(pos);
							count++;
						}
					}
					// Circle
					if(pos->type == CD_CIRCLE) {
						Vector2 p = vec2(pos->pos.x+x_off, pos->pos.y+y_off);
						if(rectf_circle_collision(rect, &p, pos->size.radius)) {
							if(callback)
								(*callback)(pos);
							count++;
						}
					}
				}
			}
		}
	}

	return count;
}

static void _coldet_cell_cast(CDWorld* cd, int cell_x, int cell_y, 
		Vector2 start, Vector2 end, uint mask, float* min_d, Vector2* hitpoint,
		CDObj** obj) {
		
	float x_off = 0.0f, y_off = 0.0f;
	CDCell* cell = _coldet_hashmap_get(cd, cell_x, cell_y, &x_off, &y_off);
	if(cell == NULL)
		return;

	CDObj* pos;
	list_for_each_entry(pos, &cell->objs, list) {
		if(pos->mask & mask) {
			Vector2 hitp;
			bool hittest = false;

			// AABB
			if(pos->type == CD_AABB) {
				RectF r = _rectf_from_aabb(pos);
				r.left += x_off; r.right += x_off;
				r.top += y_off; r.bottom += y_off;
				hitp = rectf_raycast(&r, &start, &end);
				hittest = true;
				
			}

			// Circle
			if(pos->type == CD_CIRCLE) {
				Vector2 p = vec2(pos->pos.x+x_off, pos->pos.y+y_off);
				hitp = circle_raycast(&p, pos->size.radius, &start, &end);
				hittest = true;
			}

			if(hittest && (hitp.x != end.x || hitp.y != end.y)) {
				float d = vec2_length_sq(vec2_sub(hitp, start));
				if(d < *min_d) {
					*min_d = d;
					*hitpoint = hitp;
					*obj = pos;
				}
			}
		}
	}
}

CDObj* coldet_cast_segment(CDWorld* cd, Vector2 start, Vector2 end, uint mask,
		Vector2* hitpoint) {
	assert(cd);
	assert(mask);

	Vector2 dir = vec2_normalize(vec2_sub(end, start));
	float k = dir.x / dir.y;
	float inv_k = dir.y / dir.x;

	int tile_x, tile_y;
	_pt_cell(cd, start, &tile_x, &tile_y);

	Vector2 dist, ddist = vec2(
			sqrtf(1.0f + inv_k*inv_k) * cd->cell_size,
			sqrtf(1.0f + k*k) * cd->cell_size
	);

	int step_x, step_y;

	if(dir.x >= 0.0f) {
		step_x = 1;
		dist.x = fabsf((float)(tile_x+1) * cd->cell_size - start.x);
	}
	else {
		step_x = -1;
		dist.x = fabsf(start.x - (float)tile_x * cd->cell_size);
	}

	if(dir.y >= 0.0f) {
		step_y = 1;
		dist.y = fabsf((float)(tile_y+1) * cd->cell_size - start.y);
	}
	else {
		step_y = -1;
		dist.y = fabsf(start.y - (float)tile_y * cd->cell_size);
	}

	dist.x *= ddist.x / cd->cell_size;
	dist.y *= ddist.y / cd->cell_size;

	float dx, dy;

	// Do initial check for the cell where segment start is
	float min_d = FLT_MAX;
	CDObj* obj = NULL;
	Vector2 hitp;
	
	_coldet_cell_cast(cd, tile_x-1, tile_y-1, start, end, mask, &min_d, &hitp, &obj);
	_coldet_cell_cast(cd, tile_x, tile_y-1, start, end, mask, &min_d, &hitp, &obj);
	_coldet_cell_cast(cd, tile_x-1, tile_y, start, end, mask, &min_d, &hitp, &obj);
	_coldet_cell_cast(cd, tile_x, tile_y, start, end, mask, &min_d, &hitp, &obj);
	
	// DDA
	Vector2 pos = start;
	while(!obj) {
		if(dist.x < dist.y) {
			if(step_x > 0)
				dx = (float)(tile_x+1) * cd->cell_size - pos.x;
			else
				dx = (float)tile_x * cd->cell_size - pos.x;

			pos.x += dx;
			pos.y += dx / k;

			dist.x += ddist.x;

			// x coord for next tiles to check 
			int hittest_tile_x = tile_x + (3*step_x - 1) / 2;
			tile_x += step_x;

			_coldet_cell_cast(cd, hittest_tile_x, tile_y-1, start, end, mask, &min_d, &hitp, &obj);
			_coldet_cell_cast(cd, hittest_tile_x, tile_y, start, end, mask, &min_d, &hitp, &obj);
		
			// Check if we're still inside ray segment
			if(step_x > 0) {
				if(pos.x > end.x)
					break;
			}		
			else {
				if(pos.x < end.x)
					break;
			}
		}
		else {
			if(step_y > 0)
				dy = (float)(tile_y+1) * cd->cell_size - pos.y;
			else
				dy = (float)tile_y * cd->cell_size - pos.y;

			pos.x += dy * k;
			pos.y += dy;

			dist.y += ddist.y;

			// y coord for next tiles to check
			int hittest_tile_y = tile_y + (3*step_y - 1) / 2;
			tile_y += step_y;

			_coldet_cell_cast(cd, tile_x-1, hittest_tile_y, start, end, mask, &min_d, &hitp, &obj);
			_coldet_cell_cast(cd, tile_x, hittest_tile_y, start, end, mask, &min_d, &hitp, &obj);
			
			// Check if we're still inside ray segment
			if(step_y > 0) {
				if(pos.y > end.y)
					break;
			}		
			else {
				if(pos.y < end.y)
					break;
			}
		}
	}

	if(obj)
		*hitpoint = hitp;

	return obj;
}

static void _coldet_obj_to_cell(CDWorld* cd, CDObj* obj, CDCell* cell,
		float x_off, float y_off, CDCollissionCallback callback) {
	assert(cd && obj && cell);

	CDObj* a = obj;
	CDObj* b;

	Vector2 old_pos = a->pos;
	a->pos.x -= x_off;
	a->pos.y -= y_off;

	list_for_each_entry(b, &cell->objs, list) {
		if(a >= b)
			continue;

		if(a->mask & b->mask) {
#ifndef NO_DEVMODE
			cd->last_process_hittests++;
#endif
			bool intersects = false;
			if(a->type == CD_CIRCLE && b->type == CD_CIRCLE) {
				// Circle & circle
				float r = a->size.radius + b->size.radius;
				intersects = vec2_length_sq(vec2_sub(a->pos, b->pos)) <= r*r;
			}
			else if(a->type == CD_AABB && b->type == CD_AABB) {
				// AABB & AABB
				RectF rect_a = _rectf_from_aabb(a);
				RectF rect_b = _rectf_from_aabb(b);
				intersects = rectf_rectf_collision(&rect_a, &rect_b); 
			}
			else {
				// AABB & circle
				CDObj* circle_obj = a->type == CD_CIRCLE ? a : b;
				CDObj* aabb_obj = a->type == CD_AABB ? a : b;
				RectF rect = _rectf_from_aabb(aabb_obj);
				intersects = rectf_circle_collision(&rect, &circle_obj->pos,
						circle_obj->size.radius);
			}

			if(intersects) {
				(*callback)(a, b);
			}
		}
	}

	a->pos = old_pos;
}

void coldet_process(CDWorld* cd, CDCollissionCallback callback) {
	assert(cd);

#ifndef NO_DEVMODE
	cd->last_process_hittests = 0;
	cd->last_process_reinserts = 0;
#endif

	assert(list_empty(&cd->reinsert_list));

	// Rehash dirty objects and move ones with offset
	for(uint i = 0; i < cd->reserved_cells; ++i) {
		CDCell* cell = &cd->cells[i];

	
#ifndef NO_DEVMODE
		uint cell_objs = 0;
#endif
		// Iterate over cell objects
		CDObj* obj;
		CDObj* n;
		list_for_each_entry_safe(obj, n, &cell->objs, list) {

#ifdef _DEBUG
			if(obj->type == CD_CIRCLE) {
				assert(obj->size.radius*2.0f <= cd->cell_size);
			}
			if(obj->type == CD_AABB) {
				assert(obj->size.size.x <= cd->cell_size);
				assert(obj->size.size.y <= cd->cell_size);
			}
#endif

#ifndef NO_DEVMODE
			cell_objs++;
			cd->max_objs_in_cell = MAX(cd->max_objs_in_cell, cell_objs);
#endif

			if(obj->dirty || obj->offset.x != 0.0f || obj->offset.y != 0.0f) {
				int old_tile_x, old_tile_y, new_tile_x, new_tile_y;

				_obj_cell(cd, obj, &old_tile_x, &old_tile_y);
				obj->pos = vec2_add(obj->pos, obj->offset);
				_obj_cell(cd, obj, &new_tile_x, &new_tile_y);
				obj->offset = vec2(0.0f, 0.0f);

				// Rehash obj if neccessary
				if(obj->dirty || old_tile_x != new_tile_x || old_tile_y != new_tile_y) {
					// Clear dirty flag, set new pos
					obj->dirty = false;

					// Remove obj from current list
					_coldet_hashmap_remove(cd, obj);

					// Insert into reinsert list 
					list_push_back(&cd->reinsert_list, &obj->list);
#ifndef NO_DEVMODE
					cd->last_process_reinserts++;
#endif
				}
			}
		}
	}

	// Process reinsert list
	while(!list_empty(&cd->reinsert_list)) {
		CDObj* obj = list_first_entry(&cd->reinsert_list, CDObj, list);
		list_pop_front(&cd->reinsert_list);
		_coldet_hashmap_insert(cd, obj);
	}

	// We need to check 3 to 8 cell neighbours for correct result,
	// these are neighbour offsets. The order is very important,
	// we always check [0..2], [3,4] is checked only if object 
	// crosses x boundary of cell, [5,6] if it crosses y and also 
	// 7 if it crosses both.
	const int cell_offset_x[] = {-1, 0, -1, 1, 1, -1, 0, 1};
	const int cell_offset_y[] = {-1, -1, 0, -1, 0, 1, 1, 1};
	
	// Finally, check collissions
	for(uint i = 0; i < cd->reserved_cells; ++i) {
		CDCell* cell = &cd->cells[i];
		if(list_empty(&cell->objs))
			continue;

		CDCell* neighbours[8];
		float neighbour_x_off[8] = {0};
		float neighbour_y_off[8] = {0};
		for(uint j = 0; j < 8; ++j)
			neighbours[j] = _coldet_hashmap_get(cd, 
					cell->x + cell_offset_x[j],
					cell->y + cell_offset_y[j],
					&neighbour_x_off[j],
					&neighbour_y_off[j]
			);

		// Iterate over objects
		CDObj* obj;
		list_for_each_entry(obj, &cell->objs, list) {

			// Determine if right vertical and bottom horizontal cell
			// wall is crossed.
			bool crosses_x = false, crosses_y = false;

			float local_x = obj->pos.x - (float)cell->x * cd->cell_size;
			float local_y = obj->pos.y - (float)cell->y * cd->cell_size;

			if(obj->type == CD_CIRCLE) {
				if(local_x + obj->size.radius >= cd->cell_size)
					crosses_x = true;
				if(local_y + obj->size.radius >= cd->cell_size)
					crosses_y = true;
			}

			if(obj->type == CD_AABB) {
				if(local_x + obj->size.size.x >= cd->cell_size)
					crosses_x = true;
				if(local_y + obj->size.size.y >= cd->cell_size)
					crosses_y = true;
			}

			// Iterate over objects in the same cell
			_coldet_obj_to_cell(cd, obj, cell, 0.0f, 0.0f, callback);

			// Iterate over objects in neighbour cells
			for(uint j = 0; j < 3; ++j) {
				if(neighbours[j])
					_coldet_obj_to_cell(cd, obj, neighbours[j],
							neighbour_x_off[j], neighbour_y_off[j], callback);
			}
			if(crosses_x) {
				for(uint j = 3; j < 5; ++j) {
					if(neighbours[j])
						_coldet_obj_to_cell(cd, obj, neighbours[j],
							neighbour_x_off[j], neighbour_y_off[j], callback);
				}
			}
			if(crosses_y) {
				for(uint j = 5; j < 7; ++j) {
					if(neighbours[j])
						_coldet_obj_to_cell(cd, obj, neighbours[j],
							neighbour_x_off[j], neighbour_y_off[j], callback);
				}
			}
			if(crosses_x && crosses_y) {
				if(neighbours[7])
					_coldet_obj_to_cell(cd, obj, neighbours[7],
							neighbour_x_off[7], neighbour_y_off[7], callback);
			}
		}
	}
}
