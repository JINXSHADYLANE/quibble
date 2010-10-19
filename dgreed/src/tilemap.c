#include "tilemap.h"

#include "memory.h"
#include "gfx_utils.h"

#define TILESET_WIDTH 32
#define TILESET_HEIGHT 32
#define TILES_IN_TILESET (TILESET_WIDTH * TILESET_HEIGHT)

Tilemap* tilemap_load(const char* filename) {
	assert(filename);

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
		if(w / t->tile_width != TILESET_WIDTH || 
				h / t->tile_height != TILESET_HEIGHT)
			LOG_ERROR("Bad tileset texture size");

		// Anim defs
		if((tileset->n_animdefs = file_read_uint32(f))) {
			size_t s = tileset->n_animdefs * sizeof(TileAnimDef);
			tileset->animdefs = (TileAnimDef*)MEM_ALLOC(s); 
			for(uint j = 0; j < tileset->n_animdefs; ++j) {
				TileAnimDef* animdef = &tileset->animdefs[j];	
				animdef->fps = (float)file_read_uint32(f);
				animdef->start = file_read_uint32(f);
				animdef->end = file_read_uint32(f);
				assert(animdef->start < TILES_IN_TILESET);
				assert(animdef->end < TILES_IN_TILESET);
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
			object->p.x = file_read_float(f);
			object->p.y = file_read_float(f);
		}
	}
	else
		t->objects = NULL;

	// Layers
	t->n_layers = file_read_uint32(f);
	assert(t->n_layers <= 16 && t->n_layers > 0);
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
		
		layer->render_layer = i;
	}

	// Collission
	uint compr_size = file_read_uint32(f), decompr_size = 0;
	assert(compr_size);
	void* compr = MEM_ALLOC(compr_size);
	file_read(f, compr, compr_size);
	t->collission = (byte*)lz_decompress(compr, compr_size, &decompr_size);
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
		assert(t->layers[i].name && t->layers[i].data);
		MEM_FREE(t->layers[i].name);
		MEM_FREE(t->layers[i].data);
	}
	MEM_FREE(t->layers);

	assert(t->collission);
	MEM_FREE(t->collission);
}

void _tileid_source(Tilemap* t, float time, uint16 tileid, 
		RectF* src, TexHandle* tex) {
	assert(t);
	assert(src);
	assert(tex);

	uint tileset_id = tileid / TILES_IN_TILESET;
	assert(tileset_id < t->n_tilesets);
	TilesetDef* tileset = &t->tilesets[tileset_id];
	*tex = tileset->texture;

	uint tile = tileid % TILES_IN_TILESET;

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

	uint tile_x = tile % TILESET_WIDTH;
	uint tile_y = tile / TILESET_WIDTH;

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
	float vw = rectf_width(&viewport)/2.0f * t->camera.z;
	float vh = rectf_height(&viewport)/2.0f * t->camera.z;

	// World space viewport corners
	Vector2 corners[] = {{-vw, -vh}, {vw, -vh}, {-vw, vh}, {vw, vh}};
	gfx_transform(corners, 4, &t->camera.center, t->camera.rot, 1.0f); 	

	// World space viewport bounding box
	RectF bbox = {corners[0].x, corners[0].y, corners[0].x, corners[0].y};
	for(uint i = 1; i < 4; ++i) {
		bbox.left = MIN(bbox.left, corners[i].x);
		bbox.top = MIN(bbox.top, corners[i].y);
		bbox.right = MAX(bbox.right, corners[i].x);
		bbox.bottom = MAX(bbox.bottom, corners[i].y);
	}

	// Clamp bounding box to world boundries
	bbox.left = clamp(bbox.left, 0.0f, fwidth);
	bbox.top = clamp(bbox.top, 0.0f, fheight);
	bbox.right = clamp(bbox.right, 0.0f, fwidth);
	bbox.bottom = clamp(bbox.bottom, 0.0f, fheight);

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
						&last_tileid_src, &dest, -t->camera.rot, COLOR_WHITE);
			}
		}
	}
}

Vector2 tilemap_world2screen_point(Tilemap* t, const RectF* viewport, Vector2 point) {
	assert(t);

	Vector2 half_viewport_size = vec2 (
		rectf_width(viewport)/2.0f,
		rectf_height(viewport)/2.0f
	);		

	// World space, relative to camera center
	point = vec2_sub(point, t->camera.center);

	// Screen space, relative to viewport center
	point = vec2_scale(vec2_rotate(point, t->camera.rot), 1.0f / t->camera.z);

	return vec2_add(point, half_viewport_size);
}

Vector2 tilemap_screen2world_point(Tilemap* t, const RectF* viewport, Vector2 point) {
	assert(t);

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

