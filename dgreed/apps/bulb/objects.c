#include "objects.h"

#include <tilemap.h>
#include <memory.h>

#define MAX_OBJS_IN_CELL 5

typedef struct {
	uint obj_count;
	uint obj_ids[MAX_OBJS_IN_CELL];
} SHashCell;

// Tweakables
static uint cell_size = 64;
static float fcell_size = 64.0f;
static float beacon_intensity = 180.0f;

// State
static uint world_width, world_height, cell_count;
static SHashCell* cells = NULL;
static DArray objs_array = {NULL, 0, 0, 0};
static Object* objects;
static DArray crates_array, beacons_array, buttons_array, batteries_array;
static uint* crates;
static uint* beacons;
static uint* buttons;
static uint* batteries;
static RectF player_rect;
static Tilemap* level;

static uint _vec_to_cell(Vector2 v) {
	assert(v.x >= 0.0f && v.x < (float)world_width);
	assert(v.y >= 0.0f && v.y < (float)world_width);

	uint x = (uint)(v.x / fcell_size);
	uint y = (uint)(v.y / fcell_size);
	uint cell = y * (world_width / cell_size) + x;
	assert(cell < cell_count);
	return cell;
}

static uint _aligned_rect_to_cell(RectF rect) {
	assert(rectf_width(&rect) <= fcell_size);
	assert(rectf_height(&rect) <= fcell_size);

	return _vec_to_cell(vec2(rect.left, rect.top));	
}

// Returns amount of result cells in out
static uint _rect_to_cells(RectF rect, uint out[4]) {
	assert(rectf_width(&rect) <= fcell_size);
	assert(rectf_height(&rect) <= fcell_size);

	out[0] = _vec_to_cell(vec2(rect.left, rect.top));
	out[1] = _vec_to_cell(vec2(rect.left, rect.bottom));
	out[2] = _vec_to_cell(vec2(rect.right, rect.top));
	out[3] = _vec_to_cell(vec2(rect.right, rect.bottom));

	// Optimal sorting network for 4 items
	out[0] = MIN(out[0], out[1]);
	out[1] = MAX(out[0], out[1]);
	out[2] = MIN(out[2], out[3]);
	out[3] = MAX(out[2], out[3]);
	out[0] = MIN(out[0], out[2]);
	out[1] = MIN(out[1], out[3]);
	out[2] = MAX(out[0], out[2]);
	out[3] = MAX(out[1], out[3]);
	out[1] = MIN(out[1], out[2]);
	out[2] = MAX(out[1], out[2]);

	// Remove duplicates
	uint i = 1, count = 1;
	while(i < 4) {
		bool unique = true;
		for(uint j = i+1; j < 4; ++j) {
			if(out[j] == out[i]) {
				unique = false;
				break;
			}	
		}

		if(unique) {
			count++;
			i++;
		}
		else {
			for(uint j = i+1; j < 4; ++j) 
				out[j-1] = out[j];
		}
	}

	return count;
}

void objects_reset(Tilemap* map) {
	assert(map);

	level = map;

	world_width = map->width * map->tile_width;
	world_height = map->height * map->tile_height;

	assert(world_width % cell_size == 0);
	assert(world_height % cell_size == 0);

	cell_count = (world_width / cell_size) * (world_height / cell_size); 

	if(cells)
		MEM_FREE(cells);

	cells = (SHashCell*)MEM_ALLOC(sizeof(SHashCell) * cell_count);
	memset(cells, 0, sizeof(SHashCell) * cell_count);

	if(objs_array.size != 0) {
		darray_free(&objs_array);
		darray_free(&crates_array);
		darray_free(&beacons_array);
		darray_free(&buttons_array);
		darray_free(&batteries_array);
	}

	objs_array = darray_create(sizeof(Object), level->n_objects);
	crates_array = darray_create(sizeof(uint), 0);
	beacons_array = darray_create(sizeof(uint), 0);
	buttons_array = darray_create(sizeof(uint), 0);
	batteries_array = darray_create(sizeof(uint), 0);

}

void objects_close(void) {
	MEM_FREE(cells);
	if(objs_array.size != 0) {
		darray_free(&objs_array);
		darray_free(&crates_array);
		darray_free(&beacons_array);
		darray_free(&buttons_array);
		darray_free(&batteries_array);
	}
	objs_array.size = 0;
}

void objects_add(ObjectType type, Vector2 pos) {
	assert(pos.x >= 0.0f && (pos.x + 64.0f) <= (float)world_width);
	assert(pos.y >= 0.0f && (pos.y + 64.0f) <= (float)world_height);

	Object new = {type, rectf(pos.x, pos.y, pos.x+64.0f, pos.y+64.0f)}; 
	uint i = objs_array.size;
	switch(type) {
		case obj_crate:
			darray_append(&crates_array, (void*)&i);
			break;
		case obj_beacon:
			new.beacon_taken = false;
			new.data.beacon_intensity = beacon_intensity;
			darray_append(&beacons_array, (void*)&i);
			break;
		case obj_button:
			new.data.button_state = false;
			darray_append(&buttons_array, (void*)&i);
			break;
		case obj_battery:
			new.data.battery_taken = false;
			darray_append(&batteries_array, (void*)&i);
			break;
		default:
			LOG_ERROR("Trying to add invalid object type");
	}
	darray_append(&objs_array, (void*)&new);
}

void _cell_append(uint cell_id, uint object_id) {
	assert(cell_id < cell_count);
	uint c = cells[cell_id].obj_count; 
	assert(c < MAX_OBJS_IN_CELL-1);
	cells[cell_id].obj_ids[c++] = object_id;
}

void _cell_remove(uint cell_id, uint object_id) {
	assert(cell_id < cell_count);
	SHashCell* c = &cells[cell_id];
	bool success = false;
	for(uint i = 0; i < c->obj_count; ++i) {
		if(c->obj_ids[i] == object_id) {
			success = true;
			for(uint j = i+1; j < c->obj_count; ++j) {
				c->obj_ids[j-1] = c->obj_ids[j];
			}
			c->obj_count--;
		}
	}
	assert(success);
}

void _object_move(uint object_id, RectF new_pos) {
	assert(object_id < objs_array.size);

	Object* obj = &objects[object_id];

	uint old_ccount, old_cells[4];
	uint new_ccount, new_cells[4];

	old_ccount = _rect_to_cells(obj->rect, old_cells);
	new_ccount = _rect_to_cells(new_pos, new_cells);

	obj->rect = new_pos;

	if(old_ccount == new_ccount) {
		bool equal = true;
		for(uint i = 0; i < old_ccount; ++i) {
			if(old_cells[i] != new_cells[i]) {
				equal = false;
				break;
			}
		}
		if(equal)
			return;
	}

	for(uint i = 0; i < old_ccount; ++i)
		_cell_remove(old_cells[i], object_id);

	for(uint i = 0; i < new_ccount; ++i)
		_cell_append(new_cells[i], object_id);
}

void objects_seal(RectF player) {
	player_rect = player;

	objects = DARRAY_DATA_PTR(objs_array, Object);	
	crates = DARRAY_DATA_PTR(crates_array, uint);
	beacons = DARRAY_DATA_PTR(beacons_array, uint);
	buttons = DARRAY_DATA_PTR(buttons_array, uint);
	batteries = DARRAY_DATA_PTR(batteries_array, uint);
		
	// Fill spatial hash map
	for(uint i = 0; i < objs_array.size; ++i) {
		uint cell_id = _aligned_rect_to_cell(objects[i].rect);
		_cell_append(cell_id, i);
	}

	// Assign beacons to buttons
	for(uint i = 0; i < buttons_array.size; ++i) {
		Object* button = &objects[buttons[i]];
		assert(button->type == obj_button);

		float min_d = 10000.0f;
		uint min_id = ~0;

		Vector2 bp = rectf_center(&button->rect);
		for(uint j = 0; j < beacons_array.size; ++j) {
			Object* beacon = &objects[beacons[j]];
			assert(beacon->type == obj_beacon);
			if(beacon->beacon_taken)
				continue;
			Vector2 pos = rectf_center(&beacon->rect);
			float d = vec2_length_sq(vec2_sub(pos, bp));
			if(d < min_d) {
				min_d = d;
				min_id = beacons[j];
			}
		}

		if(min_id == ~0)
			LOG_ERROR("Unable to assign beacon to a button");

		button->button_beacon = min_id;
		Object* beacon = &objects[min_id];
		assert(beacon->type == obj_beacon);
		beacon->beacon_taken = true;
		beacon->data.beacon_intensity = 0.0f;
	}
}

static bool hit_update;
static uint hit_crates_count;
static uint hit_crate_ids[16];

// Works with short rays only
static Vector2 _crates_raycast(Vector2 start, Vector2 end) {
	float min_sq_dist = 100000.0f;
	Vector2 min_hitp = end;

	Vector2 d = vec2_sub(end, start);
	Segment ray = {start, end};

	bool dir_down = d.y > 0.0f;
	bool dir_right = d.x > 0.0f;
	bool dir_up = d.y < 0.0f;
	bool dir_left = d.x < 0.0f;

	uint cell_ids[2];
	cell_ids[0] = _vec_to_cell(start);
	cell_ids[1] = _vec_to_cell(end);

	SHashCell* c;
again:
	c = &cells[cell_ids[0]];
	for(uint i = 0; i < c->obj_count; ++i) {
		Object* o = &objects[c->obj_ids[i]];
		if(o->type == obj_crate) {
			Vector2 hitp;
			RectF r = o->rect;

			Segment segs[4];
			uint n_segs = 0;

			if(dir_down)
				segs[n_segs++] = segment(vec2(r.left, r.top), vec2(r.right, r.top));
			if(dir_up)
				segs[n_segs++] = segment(vec2(r.left, r.bottom), vec2(r.right, r.bottom));
			if(dir_left)
				segs[n_segs++] = segment(vec2(r.right, r.top), vec2(r.right, r.bottom));
			if(dir_right)
				segs[n_segs++] = segment(vec2(r.left, r.top), vec2(r.left, r.bottom));

			for(uint j = 0; j < n_segs; ++j) {
				if(segment_intersect(ray, segs[j], &hitp)) {
					float sq_dist = vec2_length_sq(vec2_sub(start, hitp));	
					if(sq_dist < min_sq_dist) {
						min_sq_dist = sq_dist;
						min_hitp = hitp;

						if(hit_update) {
							// Register crate ids to quickly
							// determine which crates are being moved
							bool id_registered = false;
							for(uint i = 0; i < hit_crates_count; ++i) {
								if(hit_crate_ids[i] == c->obj_ids[i]) {
									id_registered = true;
									break;
								}
							}

							assert(hit_crates_count < 15);
							if(!id_registered)
								hit_crate_ids[hit_crates_count++] = c->obj_ids[i];
						}	
					}
				}
			}	
		}	
	}

	if(cell_ids[0] != cell_ids[1]) {
		cell_ids[0] = cell_ids[1];
		goto again;
	}

	return min_hitp;
}

static Vector2 _crates_collide_swept_rectf(RectF rect, Vector2 offset) {
	assert(rectf_width(&rect) <= fcell_size);
	assert(rectf_height(&rect) <= fcell_size);

	float res_len_sq = vec2_length_sq(offset);
	Vector2 res = offset, p1, p2;

	if(offset.x != 0.0f) {
		p1 = vec2(offset.x > 0.0f ? rect.right : rect.left, rect.top);
		p2 = vec2(offset.x > 0.0f ? rect.right : rect.left, rect.bottom);

		Vector2 int1 = _crates_raycast(p1, vec2_add(p1, offset));
		Vector2 int2 = _crates_raycast(p2, vec2_add(p2, offset));
		
		Vector2 new_off1 = vec2_sub(int1, p1);
		Vector2 new_off2 = vec2_sub(int2, p2);

		float new_off1_len_sq = vec2_length_sq(new_off1);
		float new_off2_len_sq = vec2_length_sq(new_off2);

		if(new_off1_len_sq < res_len_sq) {
			res_len_sq = new_off1_len_sq;
			res = new_off1;
		}

		if(new_off2_len_sq < res_len_sq) {
			res_len_sq = new_off2_len_sq;
			res = new_off2;
		}
	}

	if(offset.y != 0.0f) {
		p1 = vec2(rect.left, offset.y > 0.0f ? rect.bottom : rect.top);
		p2 = vec2(rect.right, offset.y > 0.0f ? rect.bottom : rect.top);

		Vector2 int1 = _crates_raycast(p1, vec2_add(p1, offset));
		Vector2 int2 = _crates_raycast(p2, vec2_add(p2, offset));
		
		Vector2 new_off1 = vec2_sub(int1, p1);
		Vector2 new_off2 = vec2_sub(int2, p2);

		float new_off1_len_sq = vec2_length_sq(new_off1);
		float new_off2_len_sq = vec2_length_sq(new_off2);

		if(new_off1_len_sq < res_len_sq) {
			res_len_sq = new_off1_len_sq;
			res = new_off1;
		}

		if(new_off2_len_sq < res_len_sq) {
			res_len_sq = new_off2_len_sq;
			res = new_off2;
		}
	}

	return res;
}

Vector2 _move_crate(uint crate_id, Vector2 offset) {
	assert(crate_id < objs_array.size);
	Object* crate = &objects[crate_id];
	assert(crate->type == obj_crate);

	RectF bbox = crate->rect;

	// x
	Vector2 dx = tilemap_collide_swept_rectf(level, bbox, vec2(offset.x, 0.0f));
	bbox.left += dx.x;
	bbox.right += dx.x;

	// y
	Vector2 dy = tilemap_collide_swept_rectf(level, bbox, vec2(0.0f, offset.y));

	offset = vec2(dx.x, dy.y);
	bbox = crate->rect;

	// Collide with crates
	dx = _crates_collide_swept_rectf(bbox, vec2(offset.x, 0.0f));
	bbox.left += dx.x;
	bbox.right += dx.x;

	dy = _crates_collide_swept_rectf(bbox, vec2(0.0f, offset.y));
	bbox.top += dy.y;
	bbox.bottom += dy.y;

	_object_move(crate_id, bbox);

	return vec2(dx.x, dy.y);
}

RectF objects_move_player(Vector2 offset, bool* battery) {
	RectF bbox = player_rect;

	*battery = false;

	// x
	Vector2 dx = tilemap_collide_swept_rectf(level, bbox, vec2(offset.x, 0.0f));
	bbox.left += dx.x;
	bbox.right += dx.x;

	// y
	Vector2 dy = tilemap_collide_swept_rectf(level, bbox, vec2(0.0f, offset.y));

	offset = vec2(dx.x, dy.y);
	RectF new_bbox = bbox;
	bbox = player_rect;

	// Collide with crates
	hit_crates_count = 0;
	hit_update = true;
	uint cmoves = 0;
	bool crate_hit = false;
	dx = _crates_collide_swept_rectf(bbox, vec2(offset.x, 0.0f));
	
	// Move crate in horizontally
	if(fabsf(dx.x) < fabsf(offset.x)) {
		hit_update = false;
		cmoves++;
		crate_hit = true;

		float last_crate_offset = 0.0f;
		for(uint i = 0; i < hit_crates_count; ++i) {
			uint crate_id = hit_crate_ids[i];
			if(rectf_rectf_collision(&new_bbox, &objects[crate_id].rect)) {
				Vector2 crate_offset = vec2(offset.x - dx.x, 0.0f);
				crate_offset = _move_crate(crate_id, crate_offset);
				last_crate_offset = crate_offset.x;
			}
		}
		bbox.left += last_crate_offset;
		bbox.right += last_crate_offset; 
	}	

	hit_crates_count = 0;
	hit_update = true;
	bbox.left += dx.x;
	bbox.right += dx.x;
	dy = _crates_collide_swept_rectf(bbox, vec2(0.0f, offset.y));

	// Move crate vertically
	if(fabsf(dy.y) < fabsf(offset.y)) {
		hit_update = false;
		cmoves++;
		crate_hit = true;

		float last_crate_offset = 0.0f;
		for(uint i = 0; i < hit_crates_count; ++i) {
			uint crate_id = hit_crate_ids[i];
			if(rectf_rectf_collision(&new_bbox, &objects[crate_id].rect)) {
				Vector2 crate_offset = vec2(0.0f, offset.y - dy.y);
				crate_offset = _move_crate(crate_id, crate_offset);
				last_crate_offset = crate_offset.y;
			}
		}
		bbox.top += last_crate_offset;
		bbox.bottom += last_crate_offset; 
	}	

	bbox.top += dy.y;
	bbox.bottom += dy.y;

	// Collide buttons against crates
	for(uint i = 0; i < buttons_array.size; ++i) {
		Object* button = &objects[buttons[i]];
		assert(button->type == obj_button);
		button->data.button_state = false;
		uint cell_id = _aligned_rect_to_cell(button->rect);
		for(uint j = 0; j < cells[cell_id].obj_count; ++j) {
			Object* obj = &objects[cells[cell_id].obj_ids[j]];
			if(obj->type == obj_crate) {
				if(rectf_rectf_collision(&obj->rect, &button->rect))
					button->data.button_state = true;
			}
		}
	}

	// Collide player with spatial hash map
	uint hit_ccount, hit_cells[4];
	hit_ccount = _rect_to_cells(bbox, hit_cells);
	for(uint i = 0; i < hit_ccount; ++i) {
		SHashCell* cell = &cells[hit_cells[i]];
		for(uint j = 0; j < cell->obj_count; ++j) {
			Object* obj = &objects[cell->obj_ids[j]];
			if(rectf_rectf_collision(&bbox, &obj->rect)) {
				if(obj->type == obj_button) {
					obj->data.button_state = true;
				}
				if(obj->type == obj_battery && !*battery && !obj->data.battery_taken) {
					*battery = true;
					obj->data.battery_taken = true;
				}
			}
		}
	}

	// Update beacon intensities
	for(uint i = 0; i < buttons_array.size; ++i) {
		Object* button = &objects[buttons[i]];
		assert(button->type == obj_button);

		Object* beacon = &objects[button->button_beacon];
		assert(beacon->type == obj_beacon);	
		assert(beacon->beacon_taken == true);

		float target_intensity = button->data.button_state ? beacon_intensity : 0.0f;
		beacon->data.beacon_intensity = lerp(beacon->data.beacon_intensity, target_intensity, 0.1f);
	}

	assert(cmoves < 2);
	return bbox;
}

