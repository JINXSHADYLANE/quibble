#include "arena.h"
#include "mml.h"
#include "memory.h"
#include "darray.h"

#define ARENA_LAYER 1

ArenaDesc current_arena_desc;

void arena_init(void) {
	current_arena_desc.platforms = NULL;
	current_arena_desc.collision_tris = NULL;
	current_arena_desc.img = MAX_UINT32;
}

void arena_close(void) {
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
}

Vector2 _deserialize_vec2(const char* serialized) {
	Vector2 result;
	sscanf(serialized, "%f,%f", &(result.x), &(result.y));
	return result;
}

Triangle _deserialize_tri(const char* serialized) {
	Triangle result;
	sscanf(serialized, "%f,%f,%f,%f,%f,%f", 
		&(result.p1.x), &(result.p1.y),
		&(result.p2.x), &(result.p2.y),
		&(result.p3.x), &(result.p3.y));
	return result;
}	

void arena_reset(const char* filename, uint n_ships) {
	arena_close();

	LOG_INFO("Loading arena from file %s", filename);

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

	// Load arena image
	NodeIdx img_node = mml_get_child(&desc, root, "img");
	if(!img_node)
		LOG_ERROR("No img propierty found in arena description");
	current_arena_desc.img = tex_load(mml_getval_str(&desc, img_node));

	// Read shadow shift
	NodeIdx shadow_node = mml_get_sibling(&desc, img_node, "shadow_shift");
	if(!shadow_node)
		LOG_ERROR("No shadow_shift propierty found in arena description");
	current_arena_desc.shadow_shift = 
		_deserialize_vec2(mml_getval_str(&desc, shadow_node));

	// Read spawnpoints
	NodeIdx spawns_node = mml_get_sibling(&desc, shadow_node, "spawnpoints");	
	if(!spawns_node)
		LOG_ERROR("No spawnpoints propierty found in arena description");
	// TODO: warn about too few spawnpoints	
	NodeIdx spawnpoint = mml_get_first_child(&desc, spawns_node);
	uint n_spawnpoints = 0;
	while(spawnpoint) {
		current_arena_desc.spawnpoints[n_spawnpoints++] =
			_deserialize_vec2(mml_getval_str(&desc, spawnpoint));
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
			_deserialize_vec2(mml_getval_str(&desc, platform));
		platform = mml_get_next(&desc, platform);
	}

	// Read collision data
	NodeIdx collision_node = mml_get_sibling(&desc, platforms_node, "collision");
	if(!collision_node)
		LOG_ERROR("No collision propierty found in arena description");
	uint tri_count = mml_count_children(&desc, collision_node);
	current_arena_desc.collision_tris = 
		(Triangle*)MEM_ALLOC(sizeof(Triangle) * tri_count);
	current_arena_desc.n_tris = tri_count;
	NodeIdx triangle = mml_get_first_child(&desc, collision_node);
	uint tri_idx = 0;
	while(triangle) {
		current_arena_desc.collision_tris[tri_idx++] =
			_deserialize_tri(mml_getval_str(&desc, triangle));
		triangle = mml_get_next(&desc, triangle);
	}

	mml_free(&desc);
}

void arena_update(float dt) {
	// TODO
}	

void arena_draw(void) {
	RectF dest = rectf(0.0f, 0.0f, 0.0f, 0.0f);
	video_draw_rect(current_arena_desc.img, ARENA_LAYER, NULL, &dest, 
		COLOR_WHITE);
}		

