#include "levels.h"

#include <mml.h>
#include <darray.h>
#include <memory.h>

static bool level_defs_allocated = false;
static bool level_mml_loaded = false;
static DArray level_defs;

static BallType _get_type(const char* typestr) {
	const struct {
		const char* str;
		BallType type;
	} ball_type_dict[] = {
		{"b", 0},
		{"w", BT_WHITE},
		{"db", BT_PAIR},
		{"dw", BT_PAIR | BT_WHITE},
		{"gb", BT_GRAV},
		{"gw", BT_GRAV | BT_WHITE},
		{"tb", BT_TIME},
		{"tw", BT_TIME | BT_WHITE}
	};

	for(uint i = 0; i < ARRAY_SIZE(ball_type_dict); ++i) {
		if(strcmp(typestr, ball_type_dict[i].str) == 0) {
			return ball_type_dict[i].type;
		}
	}
	assert(0 && "Unable to find ball type");
	return 0;
}

void levels_reset(const char* desc) {
	assert(desc);

	MMLObject level_mml;

	if(level_mml_loaded) {
		level_defs.size = 0;
		level_mml_loaded = false;	
	}

	if(!level_defs_allocated) {
		level_defs = darray_create(sizeof(LevelDef), 0);
		level_defs_allocated = true;
	}

	assert(!level_mml_loaded);
	assert(level_defs_allocated);

	// Parse mml
	char* mml_text = txtfile_read(desc);
	if(!mml_deserialize(&level_mml, mml_text))
		LOG_ERROR("Unable to parse levels desc");
	MEM_FREE(mml_text);

	// Check if root has correct name
	NodeIdx root = mml_root(&level_mml);
	if(strcmp(mml_get_name(&level_mml, root), "levels") != 0)
		LOG_ERROR("Invalid level desc");

	// Count levels
	int level_count = mml_count_children(&level_mml, root);
	if(!level_count)
		LOG_ERROR("No levels?");

	// Make sure defs darray has enough space
	if(level_defs.reserved < level_count)
		darray_reserve(&level_defs, level_count);

	// Load level defs
	NodeIdx level = mml_get_first_child(&level_mml, root);
	for(; level != 0; level = mml_get_next(&level_mml, level)) {
		LevelDef new;
		memset(&new, 0, sizeof(new));

		const char* name = mml_get_name(&level_mml, level);
		assert(strlen(name) < LEVEL_NAMELEN);
		strcpy(new.name, name);

		NodeIdx n = mml_get_child(&level_mml, level, "random_at");
		if(n)
			new.spawn_random_at = mml_getval_uint(&level_mml, n);
		
		n = mml_get_child(&level_mml, level, "random_interval");
		if(n)
			new.spawn_random_interval = mml_getval_float(&level_mml, n);

		n = mml_get_child(&level_mml, level, "spawns");
		new.n_spawns = mml_count_children(&level_mml, n);
		assert(new.n_spawns <= MAX_SPAWNS);

		for(; n != 0; n = mml_get_next(&level_mml, level)) {
			SpawnDef spawn = {
				.pos = {0.0f, 0.0f},
				.vel = {0.0f, 0.0f},
				.t = 0.0f,
				.type = _get_type(mml_get_name(&level_mml, n))
			};

			spawn.pos = mml_getval_vec2(&level_mml, n);

			NodeIdx m = mml_get_child(&level_mml, n, "vel");
			if(m)
				spawn.vel = mml_getval_vec2(&level_mml, m);
			
			m = mml_get_child(&level_mml, n, "t");
			if(m)
				spawn.t = mml_getval_float(&level_mml, m);

			new.spawns[new.n_spawns++] = spawn;
		}

		darray_append(&level_defs, &new);
	}

	mml_free(&level_mml);
	level_mml_loaded = true;
}

void levels_get(const char* name, LevelDef* def) {
	LevelDef* defs = DARRAY_DATA_PTR(level_defs, LevelDef);
	for(uint i = 0; i < level_defs.size; ++i) {
		if(strcmp(defs[i].name, name) == 0) {
			*def = defs[i];
			return;
		}
	}
	assert(0 && "Level not found");
}

void levels_close(void) {
	if(level_defs_allocated) {
		darray_free(&level_defs);
		level_defs_allocated = false;
		level_mml_loaded = false;
	}
}

