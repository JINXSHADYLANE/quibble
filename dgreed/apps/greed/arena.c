#include "arena.h"
#include "mml.h"
#include "memory.h"
#include "darray.h"
#include "objects.h"

#define ARENA_LAYER 1
#define SHADOWS_LAYER 2
#define WALLS_LAYER 4
#define CHAPTERS_FILE "greed_assets/arenas.mml"

extern bool highres;

uint total_arenas;
ChapterDesc chapters[MAX_CHAPTERS];
ArenaDesc current_arena_desc;
const char* current_arena_name = NULL;

RectF final_source = {0.0f, 0.0f, 480.0f, 320.0f};
RectF no_walls_source = {0.0f, 322.0f, 480.0f, 642.0f};
RectF walls_source = {0.0f, 644.0f, 480.0f, 964.0f};

void _parse_chapters(const char* filename) {
	assert(filename);

	memset(chapters, 0, sizeof(chapters));
	total_arenas = 0;
	
	char* desc_text = txtfile_read(filename);

	MMLObject desc;
	if(!mml_deserialize(&desc, desc_text))
		LOG_ERROR("Unable to deserialize arena list file");

	NodeIdx root = mml_root(&desc);
	if(strcmp(mml_get_name(&desc, root), "arenas") != 0)
		LOG_ERROR("Invalid arena list file");

	MEM_FREE(desc_text);	

	uint chapter_idx = 0;
	for(NodeIdx chapter = mml_get_first_child(&desc, root);
		chapter != 0;
		chapter = mml_get_next(&desc, chapter), chapter_idx++) {

		ChapterDesc* c = &chapters[chapter_idx];
		c->name = strclone(mml_getval_str(&desc, chapter));
		
		uint arena_idx = 0;
		for(NodeIdx arena = mml_get_first_child(&desc, chapter);
			arena != 0;
			arena = mml_get_next(&desc, arena), arena_idx++) {

			total_arenas++;
			
			c->arena_file[arena_idx] = 
				strclone(mml_get_name(&desc, arena));
			c->arena_players[arena_idx] = (uint)mml_getval_int(&desc, arena);	

			NodeIdx name_node = mml_get_child(&desc, arena, "name");
			assert(name_node);
			c->arena_name[arena_idx] = 
				strclone(mml_getval_str(&desc, name_node));
		}	
		c->n_arenas = arena_idx;
	}

	mml_free(&desc);	
}

void _free_chapters(void) {
	for(uint i = 0; i < MAX_CHAPTERS; ++i) {
		if(chapters[i].name == NULL)
			continue;

		MEM_FREE(chapters[i].name);
		for(uint j = 0; j < chapters[i].n_arenas; ++j) { 
			MEM_FREE(chapters[i].arena_file[j]);
			MEM_FREE(chapters[i].arena_name[j]);
		}	
	}
}

void arenas_init(void) {
	_parse_chapters(CHAPTERS_FILE);
}

void arenas_close(void) {
	_free_chapters();
}

void arena_init(void) {
	current_arena_desc.platforms = NULL;
	current_arena_desc.collision_tris = NULL;
	current_arena_desc.img = MAX_UINT32;
	current_arena_desc.density_map = NULL;
	current_arena_desc.objects.reserved = 0;
}

void _arena_prep_reset(void) {
	if(current_arena_desc.platforms) {
		MEM_FREE(current_arena_desc.platforms);
		current_arena_desc.platforms = NULL;
	}	
	if(current_arena_desc.collision_tris) {
		MEM_FREE(current_arena_desc.collision_tris);
		current_arena_desc.collision_tris = NULL;
	}
	if(current_arena_desc.img != MAX_UINT32) {
		tex_free(current_arena_desc.img);
		current_arena_desc.img = MAX_UINT32;
	}	
	if(current_arena_desc.density_map != NULL) {
		MEM_FREE(current_arena_desc.density_map);
		current_arena_desc.density_map = NULL;
	}
	if(current_arena_desc.nav_mesh.n_nodes != 0) {
		ai_free_navmesh(current_arena_desc.nav_mesh);
		current_arena_desc.nav_mesh.n_nodes = 0;
	}	
	if(current_arena_desc.objects.reserved != 0) {
		darray_free(&current_arena_desc.objects);
		current_arena_desc.objects.reserved = 0;
	}
}

void arena_close(void) {
	_arena_prep_reset();
}

void arena_reset(const char* filename, uint n_ships) {
	_arena_prep_reset();

	LOG_INFO("Loading arena from file %s", filename);
	current_arena_name = filename;

	char* desc_text = txtfile_read(filename);
	if(!desc_text)
		LOG_ERROR("Unable to read arena description file %s", filename);

	MMLObject desc;
	if(!mml_deserialize(&desc, desc_text))
		LOG_ERROR("Failed to deserialize arena description");

	MEM_FREE(desc_text);	

	NodeIdx root = mml_root(&desc);
	if(strcmp(mml_get_name(&desc, root), "arena") != 0)
		LOG_ERROR("Invalid arena description file");

	// Load composite arena image
	NodeIdx walls_node = mml_get_child(&desc, root, "walls");
	if(!walls_node)
		LOG_ERROR("No walls propierty found in arena description");
	current_arena_desc.img = 
		tex_load(mml_getval_str(&desc, walls_node));

	// Read precalculated data filename
	NodeIdx precalc_node = mml_get_child(&desc, root, "precalc");
	if(!precalc_node)
		LOG_ERROR("No precalc propierty found in arena description");
	FileHandle precalc_file = file_open(mml_getval_str(&desc, precalc_node));	

	// Load collision data
	uint32 n = file_read_uint32(precalc_file);
	current_arena_desc.n_tris = n;
	if(n > 0) {
		current_arena_desc.collision_tris = 
			(Triangle*)MEM_ALLOC(sizeof(Triangle)*n);
		Triangle* tris = current_arena_desc.collision_tris;	
		for(uint i = 0; i < n; ++i) {
			tris[i].p1.x = file_read_float(precalc_file);	
			tris[i].p1.y = file_read_float(precalc_file);	
			tris[i].p2.x = file_read_float(precalc_file);	
			tris[i].p2.y = file_read_float(precalc_file);	
			tris[i].p3.x = file_read_float(precalc_file);	
			tris[i].p3.y = file_read_float(precalc_file);	
		}
	}

	// Load density map
	n = file_read_uint32(precalc_file);
	if(n > 0) {
		assert(n == DENSITY_MAP_WIDTH * DENSITY_MAP_HEIGHT);	
		current_arena_desc.density_map = MEM_ALLOC(n * sizeof(float));
		for(uint i = 0; i < n; ++i)
			current_arena_desc.density_map[i] = file_read_float(precalc_file);
	}

	// Load navmesh
	current_arena_desc.nav_mesh = ai_load_navmesh(precalc_file);
	file_close(precalc_file);
	
	// Read no shadow flag
	current_arena_desc.no_wall_shadows = false;
	NodeIdx no_wall_shadows_node = mml_get_sibling(&desc, precalc_node, 
			"no_wall_shadows");
	if(no_wall_shadows_node)
		current_arena_desc.no_wall_shadows = mml_getval_bool(&desc, 
				no_wall_shadows_node);

	// Read shadow shift
	NodeIdx shadow_node = mml_get_sibling(&desc, precalc_node, 
			"shadow_shift");
	if(!shadow_node)
		LOG_ERROR("No shadow_shift propierty found in arena description");
	current_arena_desc.shadow_shift = mml_getval_vec2(&desc, shadow_node);

	// Read spawnpoints
	NodeIdx spawns_node = mml_get_sibling(&desc, shadow_node, "spawnpoints");	
	if(!spawns_node)
		LOG_ERROR("No spawnpoints propierty found in arena description");
	// TODO: warn about too few spawnpoints	
	NodeIdx spawnpoint = mml_get_first_child(&desc, spawns_node);
	uint n_spawnpoints = 0;
	while(spawnpoint) {
		current_arena_desc.spawnpoints[n_spawnpoints++] = 
			mml_getval_vec2(&desc, spawnpoint);
		if(n_spawnpoints > MAX_SHIPS)
			LOG_ERROR("Too many spawnpoints. Only %d are needed", MAX_SHIPS);
		spawnpoint = mml_get_next(&desc, spawnpoint);
	}

	// Read platforms
	NodeIdx platforms_node = mml_get_sibling(&desc, spawns_node, "platforms");
	if(!platforms_node)
		LOG_ERROR("No platforms propierty found in arena description");
	uint platform_count = mml_count_children(&desc, platforms_node);
	current_arena_desc.platforms = 
		(Vector2*)MEM_ALLOC(sizeof(Vector2) * platform_count);
	current_arena_desc.n_platforms = platform_count;
	NodeIdx platform = mml_get_first_child(&desc, platforms_node);
	uint platform_idx = 0;
	while(platform) {
		current_arena_desc.platforms[platform_idx++] = 
			mml_getval_vec2(&desc, platform);
		platform = mml_get_next(&desc, platform);
	}

	// Read objects
	NodeIdx objects_node = mml_get_sibling(&desc, platforms_node, "objects");
	if(objects_node)
		objects_load(&desc, objects_node, &current_arena_desc.objects);
	else 
		// No objects in mml, create empty darray (for editmode)
		current_arena_desc.objects = darray_create(sizeof(Object), 0);

	objects_reset(&current_arena_desc.objects);

	mml_free(&desc);
}

void arena_update(float dt) {
	// TODO
}	

void arena_draw(RectF* obstructions, uint obstruction_count) {
	RectF rects[48];
	uint cnt = 0;
	rects[cnt++] = final_source;

	// Perform CSG & clipping for all obstructions
	for(uint i = 0; i < obstruction_count; ++i) {
		uint old_cnt = cnt;
		for(uint j = 0; j < old_cnt; ++j) {
			RectF out[4];
			uint out_cnt = rectf_cut(&rects[j], &obstructions[i], out);
			for(uint k = 0; k < out_cnt; ++k) {
				rects[cnt++] = out[k];
				assert(cnt < 48);
			}	
		}
		for(uint j = old_cnt; j < cnt; ++j) {
			rects[j - old_cnt] = rects[j];
		}
		cnt -= old_cnt;
	}
<<<<<<< local
=======
	
	// Only negative shifts supported for now
	//assert(shift.x < 0.0f && shift.y < 0.0f);
>>>>>>> other

	// Draw unobstructed arena parts
	for(uint i = 0; i < cnt; ++i) {
		video_draw_rect(current_arena_desc.img, ARENA_LAYER, 
			&rects[i], &rects[i], COLOR_WHITE);
	}

	// Draw parts in front of obstructions
	for(uint i = 0; i < obstruction_count; ++i) {
		RectF src = obstructions[i];
		src.top += final_source.bottom + 2.0f;
		src.bottom += final_source.bottom + 2.0f;
		video_draw_rect(current_arena_desc.img, ARENA_LAYER,
			&src, &obstructions[i], COLOR_WHITE);

		src.top += final_source.bottom + 2.0f;
		src.bottom += final_source.bottom + 2.0f;
		video_draw_rect(current_arena_desc.img, WALLS_LAYER,
			&src, &obstructions[i], COLOR_WHITE);
	}

	objects_render(time_ms() / 1000.0f, 255);
}		

void arena_draw_transition(float t) {
	Color c = color_lerp(COLOR_WHITE, COLOR_WHITE&0xFFFFFF, t);
	int alpha = c >> 24;

	RectF dest = rectf(0.0f, 0.0f, 0.0f, 0.0f);
	video_draw_rect(current_arena_desc.img, ARENA_LAYER, 
		&final_source, &dest, c);

	objects_render(time_ms() / 1000.0f, alpha);
}

const char* arena_get_current(void) {
	assert(current_arena_name);
	return current_arena_name;
}

const char* arena_get_next(void) {
	assert(current_arena_name);	

	// Find current arena, return next non-empty arena
	for(uint i = 0; i < MAX_CHAPTERS; ++i) {
		for(uint j = 0; j < MAX_ARENAS_IN_CHAPTER; ++j) {
			if(chapters[i].arena_file[j]) {
				if(strcmp(current_arena_name, chapters[i].arena_file[j]) == 0) {
					do {
						while(++j < MAX_ARENAS_IN_CHAPTER) {
							if(chapters[i].arena_name[j])
								return chapters[i].arena_file[j];
						}
						j = ~0;
					} while(++i < MAX_CHAPTERS);
					goto end;
				}
			}
		}
	}
end:
	LOG_WARNING("Unable to get next arena after %s", current_arena_name);
	return NULL;
}

uint arena_closest_navpoint(Vector2 pos) {
	return ai_nearest_navpoint(&current_arena_desc.nav_mesh, pos);
}

uint arena_platform_navpoint(uint platform) {
	assert(platform < n_platforms);
	Vector2 pos = current_arena_desc.platforms[platform];
	return ai_nearest_navpoint(&current_arena_desc.nav_mesh, pos);
}	

uint arena_platform_nearby_navpoint(uint platform) {
	uint search_depth = 3;
	NavMesh* navmesh = &current_arena_desc.nav_mesh;

	// Do a random graph walk
	uint current_node = arena_platform_navpoint(platform);
	while(search_depth--) {
		uint i = navmesh->neighbours_start[current_node] 
			+ rand_int(0, navmesh->neighbour_count[current_node]);
		current_node = navmesh->neighbours[i];
	}

	return current_node;
}

