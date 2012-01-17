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

static void _parse_level(MMLObject* mml, NodeIdx node, LevelDef* dest) {
	const char* name = mml_getval_str(mml, node);
	assert(strlen(name) < LEVEL_NAMELEN);
	strcpy(dest->name, name);

	NodeIdx n = mml_get_child(mml, node, "random_at");
	if(n)
		dest->spawn_random_at = mml_getval_uint(mml, n);
	
	n = mml_get_child(mml, node, "random_interval");
	if(n)
		dest->spawn_random_interval = mml_getval_float(mml, n);

	n = mml_get_child(mml, node, "spawns");
	dest->n_spawns = 0; 
	assert(dest->n_spawns <= MAX_SPAWNS);

	n = mml_get_first_child(mml, n);
	for(; n != 0; n = mml_get_next(mml, n)) {
		SpawnDef spawn = {
			.pos = {0.0f, 0.0f},
			.vel = {0.0f, 0.0f},
			.t = 0.0f,
			.s = 0.0f,
			.type = _get_type(mml_get_name(mml, n))
		};

		spawn.pos = mml_getval_vec2(mml, n);

		NodeIdx m = mml_get_child(mml, n, "vel");
		if(m)
			spawn.vel = mml_getval_vec2(mml, m);
		
		m = mml_get_child(mml, n, "t");
		if(m)
			spawn.t = mml_getval_float(mml, m);

		m = mml_get_child(mml, n, "s");
		if(m)
			spawn.s = mml_getval_float(mml, m);

		assert(dest->n_spawns < MAX_SPAWNS);
		dest->spawns[dest->n_spawns++] = spawn;
	}
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

		_parse_level(&level_mml, level, &new);
		
		darray_append(&level_defs, &new);
	}

	mml_free(&level_mml);
	level_mml_loaded = true;
}

void levels_parse_ed(const char* desc) {
	assert(desc);

	MMLObject ed_mml;

	// Parse mml
	char* mml_text = txtfile_read(desc);
	if(!mml_deserialize(&ed_mml, mml_text))
		LOG_ERROR("Unable to parse editor level");
	MEM_FREE(mml_text);

	// Check root name
	NodeIdx root = mml_root(&ed_mml);
	if(strcmp(mml_get_name(&ed_mml, root), "edlevel") != 0)
		LOG_ERROR("Invalid level desc");

	// Check level count, there should be only one
	int level_count = mml_count_children(&ed_mml, root);
	if(level_count != 1)
		LOG_ERROR("How many levels did you put in edlevel.mml?");

	// Get level node, name
	NodeIdx level = mml_get_first_child(&ed_mml, root);
	const char* name = mml_getval_str(&ed_mml, level);

	// Find old level
	LevelDef* new = NULL;
	LevelDef* defs = DARRAY_DATA_PTR(level_defs, LevelDef);
	for(uint i = 0; i < level_defs.size; ++i) {
		if(strcmp(defs[i].name, name) == 0) {
			new = &defs[i];
			break;
		}
	}

	// Load the level
	assert(new);
	memset(new, 0, sizeof(LevelDef));
	_parse_level(&ed_mml, level, new);
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

