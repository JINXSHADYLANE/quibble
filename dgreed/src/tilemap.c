#include "tilemap.h"

#include "memory.h"
#include "gfx_utils.h"

#define MAX_TILESET_WIDTH 32
#define MAX_TILESET_HEIGHT 32
#define MAX_TILES_IN_TILESET (MAX_TILESET_WIDTH * MAX_TILESET_HEIGHT)

Tilemap* tilemap_load(const char* filename) {
	assert(filename);

	LOG_INFO("Loading tilemap from file %s", filename);

	FileHandle f = file_open(filename);
	if(file_read_uint32(f) != FOURCC('B', 'T', 'M', '0')) 
		LOG_ERROR("Invalid btm file");

	Tilemap* t = (Tilemap*)MEM_ALLOC(sizeof(Tilemap));
	
	// Stats
	t->tile_width = (uint)file_read_uint16(f);
	t->tile_height = (uint)file_read_uint16(f);
	t->width = file_read_uint32(f);
	t->height = file_read_uint32(f);

	// Tilesets
	t->n_tilesets = file_read_uint32(f);
	assert(t->n_tilesets);
	size_t s = t->n_tilesets * sizeof(TilesetDef);
	t->tilesets = (TilesetDef*)MEM_ALLOC(s);
	for(uint i = 0; i < t->n_tilesets; ++i) {
		TilesetDef* tileset = &t->tilesets[i];

		// Texture
		uint tex_filename_len = file_read_uint32(f);
		assert(tex_filename_len < 63 && tex_filename_len > 0);
		char tex_filename[64];
		file_read(f, tex_filename, tex_filename_len);
		tex_filename[tex_filename_len] = '\0';
		tileset->texture = tex_load(tex_filename);
		uint w, h;
		tex_size(tileset->texture, &w, &h);
		if(w % t->tile_width != 0 || h % t->tile_height != 0)
			LOG_ERROR("Tileset texture size should be divisable by tile size");
		tileset->width = w / t->tile_width;
		tileset->height = h / t->tile_height;
		if(tileset->width > MAX_TILESET_WIDTH ||
			tileset->height > MAX_TILESET_HEIGHT)
			LOG_ERROR("Too many tiles in tileset %d", i);

		// Anim defs
		if((tileset->n_animdefs = file_read_uint32(f))) {
			size_t s = tileset->n_animdefs * sizeof(TileAnimDef);
			tileset->animdefs = (TileAnimDef*)MEM_ALLOC(s); 
			for(uint j = 0; j < tileset->n_animdefs; ++j) {
				TileAnimDef* animdef = &tileset->animdefs[j];	
				animdef->fps = (float)file_read_uint32(f);
				animdef->start = file_read_uint32(f);
				animdef->end = file_read_uint32(f);
				assert(animdef->start < MAX_TILES_IN_TILESET);
				assert(animdef->end < MAX_TILES_IN_TILESET);
			}
		}	
		else
			tileset->animdefs = NULL;
	}

	// Objects
	if((t->n_objects = file_read_uint32(f))) {
		size_t s = t->n_objects * sizeof(TilemapObject);
		t->objects = (TilemapObject*)MEM_ALLOC(s);
		for(uint i = 0; i < t->n_objects; ++i) {
			TilemapObject* object = &t->objects[i];
			object->id = file_read_uint32(f);
			object->p.x = (float)file_read_uint32(f) * t->tile_width;
			object->p.y = (float)file_read_uint32(f) * t->tile_height;
		}
	}
	else
		t->objects = NULL;

	// Layers
	t->n_layers = file_read_uint32(f);
	assert(t->n_layers <= 16 && t->n_layers > 0);
	s = t->n_layers * sizeof(TilemapLayer);
	t->layers = (TilemapLayer*)MEM_ALLOC(s);
	for(uint i = 0; i < t->n_layers; ++i) {
		TilemapLayer* layer = &t->layers[i];

		// Name
		uint name_len = file_read_uint32(f);
		assert(name_len);
		layer->name = (char*)MEM_ALLOC(name_len+1);
		file_read(f, layer->name, name_len);
		layer->name[name_len] = '\0';

		// Data
		uint compr_size = file_read_uint32(f), decompr_size = 0;
		assert(compr_size);
		void* compr = MEM_ALLOC(compr_size);	
		file_read(f, compr, compr_size);
		layer->data = (uint16*)lz_decompress(compr, compr_size, &decompr_size);

		assert(decompr_size == t->width * t->height * sizeof(uint16));
		MEM_FREE(compr);

		// Swap on big-endian
		if(0x0011 != endian_swap2(0x0011)) {
			for(size_t j = 0; j < t->width * t->height; ++j) {
				layer->data[j] = endian_swap2(layer->data[j]);
			}
		}
		
		layer->render_layer = i;
	}

	// Collission
	uint compr_size = file_read_uint32(f), decompr_size = 0;
	assert(compr_size);
	void* compr = MEM_ALLOC(compr_size);
	file_read(f, compr, compr_size);
	t->collision = (byte*)lz_decompress(compr, compr_size, &decompr_size);
	uint expected_size = (t->width * t->height) / 8;
	if(expected_size * 8 != t->width * t->height)
		expected_size++;
	assert(decompr_size == expected_size); 
	MEM_FREE(compr);

	// Camera
	t->camera.center.x = (float)(t->width * t->tile_width)/2;
	t->camera.center.y = (float)(t->height * t->tile_height)/2;
	t->camera.z = 1.0f;
	t->camera.rot = 0.0f;

	file_close(f);
	return t;
}

Tilemap* tilemap_new(
	uint tile_width, uint tile_height,
	uint width, uint height, uint layers
) {
	assert(layers && tile_width && tile_height && width && height);

	Tilemap* t = (Tilemap*)MEM_ALLOC(sizeof(Tilemap));
	memset(t, 0, sizeof(Tilemap));

	t->tile_width = tile_width;
	t->tile_height = tile_height;
	t->width = width;
	t->height = height;
	t->n_layers = layers;
	t->layers = (TilemapLayer*)MEM_ALLOC(sizeof(TilemapLayer) * layers);
	memset(t->layers, 0, sizeof(TilemapLayer) * layers);

	size_t collision_size = (width * height) / 8;
	if(collision_size * 8 != width * height)
		collision_size++;

	t->collision = (byte*)MEM_ALLOC(collision_size); 
	memset(t->collision, 0, collision_size);

	for(uint i = 0; i < layers; ++i) {
		TilemapLayer* l = &t->layers[i];
		l->data = (uint16*)MEM_ALLOC(width * height * 2);
		memset(l->data, 0, width * height * 2);
		l->render_layer = 1 + i;
	}

	return t;
}

void tilemap_free(Tilemap* t) {
	assert(t);

	// Tilesets
	assert(t->n_tilesets && t->tilesets);
	for(uint i = 0; i < t->n_tilesets; ++i) {
		tex_free(t->tilesets[i].texture);
		if(t->tilesets[i].n_animdefs) {
			assert(t->tilesets[i].animdefs);
			MEM_FREE(t->tilesets[i].animdefs);
		}
	}
	MEM_FREE(t->tilesets);

	// Objects
	if(t->n_objects) {
		assert(t->objects);
		MEM_FREE(t->objects);
	}

	// Layers
	assert(t->n_layers && t->layers);
	for(uint i = 0; i < t->n_layers; ++i) {
		assert(t->layers[i].data);
		if(t->layers[i].name)
			MEM_FREE(t->layers[i].name);
		MEM_FREE(t->layers[i].data);
	}
	MEM_FREE(t->layers);

	assert(t->collision);
	MEM_FREE(t->collision);

	MEM_FREE(t);
}

static void _tileid_source(Tilemap* t, float time, uint16 tileid, 
		RectF* src, TexHandle* tex) {
	assert(t);
	assert(src);
	assert(tex);

	uint tileset_id = tileid / MAX_TILES_IN_TILESET;
	assert(tileset_id < t->n_tilesets);
	TilesetDef* tileset = &t->tilesets[tileset_id];
	*tex = tileset->texture;

	uint tile = tileid % MAX_TILES_IN_TILESET;

	// TODO: Make this quicker
	for(uint i = 0; i < tileset->n_animdefs; ++i) {
		TileAnimDef* animdef = &tileset->animdefs[i];
		if(tile >= animdef->start && tile <= animdef->end) {
			uint n_frames = animdef->end - animdef->start + 1;
			uint frame = (uint)(time / animdef->fps);
			frame %= n_frames;
			tile = animdef->start + ((tile - animdef->start) + frame) % n_frames;
			break;
		}
	}

	uint tile_x = tile % tileset->width; 
	uint tile_y = tile / tileset->width;

	src->left = (float)(tile_x * t->tile_width);
	src->top = (float)(tile_y * t->tile_height);
	src->right = src->left + (float)t->tile_width;
	src->bottom = src->top + (float)t->tile_height;
}

void tilemap_render(Tilemap* t, RectF viewport, float time) {
	assert(t);

	// World width and height in pixels
	float fwidth = (float)(t->width * t->tile_width);
	float fheight = (float)(t->height * t->tile_height);

	// World space viewport half-width and half-height
	float vw = rectf_width(&viewport)/(2.0f * t->camera.z);
	float vh = rectf_height(&viewport)/(2.0f * t->camera.z);

	Vector2 camera_pos = t->camera.center;

	// World space viewport corners
	Vector2 corners[] = {{-vw, -vh}, {vw, -vh}, {-vw, vh}, {vw, vh}};
	gfx_transform(corners, 4, &camera_pos, t->camera.rot, 1.0f); 	

	// World space viewport bounding box
	RectF bbox = {corners[0].x, corners[0].y, corners[0].x, corners[0].y};
	for(uint i = 1; i < 4; ++i) {
		bbox.left = MIN(bbox.left, corners[i].x);
		bbox.top = MIN(bbox.top, corners[i].y);
		bbox.right = MAX(bbox.right, corners[i].x);
		bbox.bottom = MAX(bbox.bottom, corners[i].y);
	}

	// Clamp bounding box to world boundries
	bbox.left = clamp(0.0f, fwidth, bbox.left);
	bbox.top = clamp(0.0f, fheight, bbox.top);
	bbox.right = clamp(0.0f, fwidth, bbox.right);
	bbox.bottom = clamp(0.0f, fheight, bbox.bottom);

	// Convert viewport bbox to to tile x and y ranges
	uint min_tile_x = (uint)bbox.left / t->tile_width;
	uint max_tile_x = (uint)bbox.right / t->tile_width;
	uint min_tile_y = (uint)bbox.top / t->tile_height;
	uint max_tile_y = (uint)bbox.bottom / t->tile_height;

	// Cache, for source of last tileid
	// TODO: Cache multiple items ?
	uint16 last_tileid = 0;
	RectF last_tileid_src = rectf_null();
	TexHandle last_tileid_tex = 0;
	
	for(uint y = min_tile_y; y <= max_tile_y; ++y) {
		for(uint x = min_tile_x; x <= max_tile_x; ++x) {
			if(x >= t->width || y >= t->height)
				continue;

			uint idx = IDX_2D(x, y, t->width);

			// Skip, if tile is empty in all layers.
			bool empty = true;
			for(uint l = 0; l < t->n_layers; ++l) {
				if(t->layers[l].data[idx] != 0) {
					empty = false;
					break;
				}
			}
			if(empty)
				continue;

			// World-space center of current tile
			Vector2 wl_center = vec2 (
				(float)((2*x+1) * t->tile_width) / 2.0f,
				(float)((2*y+1) * t->tile_height) / 2.0f
			);		
			// Calculate screen-space dest rect
			Vector2 ss_center = tilemap_world2screen_point(t, &viewport, wl_center);
			float ss_hwidth = (float)t->tile_width * t->camera.z / 2.0f;
			float ss_hheight = (float)t->tile_height * t->camera.z / 2.0f;
			RectF dest = rectf (
				ss_center.x - ss_hwidth,
				ss_center.y - ss_hheight,
				ss_center.x + ss_hwidth,
				ss_center.y + ss_hheight
			);		

			// Actual rendering
			for(uint l = 0; l < t->n_layers; ++l) {
				uint tileid = t->layers[l].data[idx];
				if(!tileid)
					continue;
				
				if(tileid != last_tileid) {
					last_tileid = tileid;
					_tileid_source(t, time, tileid, 
							&last_tileid_src, &last_tileid_tex);
				}	

				video_draw_rect_rotated(last_tileid_tex, t->layers[l].render_layer,
						&last_tileid_src, &dest, t->camera.rot, COLOR_WHITE);
			}
		}
	}

}

static uint _pos_to_tile(Tilemap* t, Vector2 pos) {
	assert(t);
	assert(pos.x >= 0.0f && pos.x <= (float)t->width * t->tile_width);
	assert(pos.y >= 0.0f && pos.y <= (float)t->height * t->tile_height);

	uint x = pos.x / t->tile_width;
	uint y = pos.y / t->tile_height;

	return IDX_2D(x, y, t->width);
}

static bool _is_solid(Tilemap* t, int x, int y) {
	assert(t);
	if(x < 0 || x >= t->width || y < 0 || y >= t->height)
		return true;
	
	uint tile = IDX_2D(x, y, t->width);
	return (t->collision[tile/8] & (1 << (7-(tile % 8)))) != 0;
}

bool tilemap_is_solid(Tilemap* t, int x, int y) {
	return _is_solid(t, x, y);
}

bool tilemap_collide(Tilemap* t, RectF rect) {
	assert(t);

	float rw = rectf_width(&rect) - 1.0f;
	float rh = rectf_height(&rect) - 1.0f;

	// Cover rect with a grid of points, if distances dx between point rows and
	// and dy between columns are not greater than tile width / height, it is 
	// sufficient to only check this grid and nothing more.

	float fcols = ceilf(rw / (float)t->tile_width);
	float frows = ceilf(rh / (float)t->tile_height);
	float dx = rw / fcols; 	
	float dy = rh / frows;

	assert(dx < (float)t->tile_width);
	assert(dy < (float)t->tile_height);

	// Check grid points
	Vector2 p = vec2(0.0f, rect.top);
	for(uint i = 0; (float)i < frows+1.0f; ++i) { 
		p.x = rect.left;
		for(uint j = 0; (float)j < fcols+1.0f; ++j) {
			if(tilemap_collide_point(t, p))
				return true;
			p.x += dx;
		}
		p.y += dy;
	}	
	return false;

}

bool tilemap_collide_point(Tilemap* t, Vector2 point) {
	assert(t);

	return _is_solid(t, 
		point.x / (float)t->tile_width,
		point.y / (float)t->tile_height
	);
}

Vector2 tilemap_raycast(Tilemap* t, Vector2 start, Vector2 end) {
	assert(t);

	// Wolf3d style DDA raycast

	uint tile = _pos_to_tile(t, start);

	int step_x, step_y, map_x, map_y;
	map_x = tile % t->width;
	map_y = tile / t->width;
	
	Vector2 dir = vec2_sub(end, start);
	dir = vec2_normalize(dir);
	float k = dir.x / dir.y;

	Vector2 delta_dist = vec2 (
		sqrtf(1.0f + dir.y * dir.y / (dir.x * dir.x)) * (float)t->tile_width,
		sqrtf(1.0f + dir.x * dir.x / (dir.y * dir.y)) * (float)t->tile_height
	);	

	Vector2 side_dist;

	if(dir.x > 0.0f) {
		step_x = 1;
		side_dist.x = (float)((map_x+1) * t->tile_width) - start.x;
		side_dist.x *= delta_dist.x / t->tile_width;
	}
	else {
		step_x = -1;
		side_dist.x = start.x - (float)(map_x * t->tile_width);
		side_dist.x *= delta_dist.x / t->tile_width;
	}
	if(dir.y > 0.0f) {
		step_y = 1;
		side_dist.y = (float)((map_y+1) * t->tile_height) - start.y;
		side_dist.y *= delta_dist.y / t->tile_width;
	}
	else {
		step_y = -1;
		side_dist.y = start.y - (float)(map_y * t->tile_height);
		side_dist.y *= delta_dist.y / t->tile_width;
	}

	float dx, dy;
	
	// Actual DDA
	Vector2 pos = start;
	while(!_is_solid(t, map_x, map_y)) {
		if(side_dist.x < side_dist.y) {
			if(step_x > 0) 
				dx = (float)((map_x+1) * t->tile_width) - pos.x;
			else
				dx = (float)(map_x * t->tile_width) - pos.x;
			pos.x += dx;
			pos.y += dx / k;

			side_dist.x += delta_dist.x;
			map_x += step_x;
		}
		else {
			if(step_y > 0) 
				dy = (float)((map_y+1) * t->tile_height) - pos.y;
			else
				dy = (float)(map_y * t->tile_width) - pos.y;
			pos.x += dy * k;
			pos.y += dy;
		
			side_dist.y += delta_dist.y;
			map_y += step_y;
		}

		if(step_x > 0) {
			if(pos.x > end.x)
				return end;
		}		
		else {
			if(pos.x < end.x)
				return end;
		}		

		if(step_y > 0) {
			if(pos.y > end.y)
				return end;
		}		
		else {
			if(pos.y < end.y)
				return end;
		}

	}

	return pos;
}

Vector2 tilemap_collide_swept_rectf(Tilemap* t, RectF rect, Vector2 offset) {
	assert(t);

	// Cover rect with a grid of points, like in collide_rect.

	float rw = rectf_width(&rect) - 1.0f;
	float rh = rectf_height(&rect) - 1.0f;
	float fcols = ceilf(rw / (float)t->tile_width);
	float frows = ceilf(rh / (float)t->tile_height);
	float dx = rw / fcols; 	
	float dy = rh / frows;

	assert(dx < (float)t->tile_width);
	assert(dy < (float)t->tile_height);

	// If we assume, that initially rect is not colliding with anything,
	// it is not neccessary to cast rays from each point in the grid.
	// Only points lying on an edge, which is moving face-forward towards
	// destination, must be checked.

	float res_len_sq = vec2_length_sq(offset);
	Vector2 res = offset, p;

	// Vertical edges
	if(offset.x != 0.0f) {
		p = vec2(offset.x > 0.0f ? rect.right : rect.left, rect.top);
		for(uint i = 0; (float)i < frows+1.0f; ++i) {
			Vector2 intersection = tilemap_raycast(t, p, vec2_add(p, offset));
			Vector2 new_offset = vec2_sub(intersection, p);
			float new_offset_len_sq = vec2_length_sq(new_offset);
			if(new_offset_len_sq < res_len_sq) {
				res_len_sq = new_offset_len_sq;
				res = new_offset;
			}
			p.y += dy;
		}
	}

	// Horizontal edges
	if(offset.y != 0.0f) {

		// Skip points, which were checked during vertical phase
		if(offset.x > 0.0f) {
			rect.left += dx;
			fcols -= 1.0f;
		}
		if(offset.x < 0.0f) {
			fcols -= 1.0f;
		}	
			
		p = vec2(rect.left, offset.y > 0.0f ? rect.bottom : rect.top);
		for(uint i = 0; (float)i < fcols+1.0f; ++i) {
			Vector2 intersection = tilemap_raycast(t, p, vec2_add(p, offset));
			Vector2 new_offset = vec2_sub(intersection, p);
			float new_offset_len_sq = vec2_length_sq(new_offset);
			if(new_offset_len_sq < res_len_sq) {
				res_len_sq = new_offset_len_sq;
				res = new_offset;
			}
			p.x += dx;
		}
	}

	return res;
}

RectF tilemap_world2screen(Tilemap* t, const RectF* viewport, RectF rect) {
	assert(t);
	assert(viewport);

	Vector2 center = vec2((rect.left + rect.right) / 2.0f, (rect.top + rect.bottom) / 2.0f);	
	center = tilemap_world2screen_point(t, viewport, center);

	float rw = rectf_width(&rect) / 2.0f * t->camera.z;
	float rh = rectf_height(&rect) / 2.0f * t->camera.z;

	return rectf(center.x - rw, center.y - rh, center.x + rw, center.y + rh);
}

RectF tilemap_screen2world(Tilemap *t, const RectF* viewport, RectF rect) {
	assert(t);
	assert(viewport);

	Vector2 center = vec2((rect.left + rect.right) / 2.0f, (rect.top + rect.bottom) / 2.0f);
	center = tilemap_screen2world_point(t, viewport, center);

	float rw = rectf_width(&rect) / 2.0f / t->camera.z;
	float rh = rectf_height(&rect) / 2.0f / t->camera.z;

	return rectf(center.x - rw, center.y - rh, center.x + rw, center.y + rh);
}

Vector2 tilemap_world2screen_point(Tilemap* t, const RectF* viewport, Vector2 point) {
	assert(t);
	assert(viewport);

	Vector2 half_viewport_size = vec2 (
		rectf_width(viewport)/2.0f,
		rectf_height(viewport)/2.0f
	);		

	// World space, relative to camera center
	point = vec2_sub(point, t->camera.center);

	// Screen space, relative to viewport center
	point = vec2_scale(vec2_rotate(point, t->camera.rot), t->camera.z);

	return vec2_add(point, half_viewport_size);
}

Vector2 tilemap_screen2world_point(Tilemap* t, const RectF* viewport, Vector2 point) {
	assert(t);
	assert(viewport);

	Vector2 half_viewport_size = vec2 (
		rectf_width(viewport)/2.0f,
		rectf_height(viewport)/2.0f
	);		

	// Screen space, relative to viewport center
	point = vec2_sub(point, half_viewport_size);

	// World space, relative to camera center
	point = vec2_rotate(vec2_scale(point, t->camera.z), -t->camera.rot);

	return vec2_add(point, t->camera.center);
}

