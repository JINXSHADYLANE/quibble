#include "levels.h"

#include "achievements.h"

#include <mml.h>
#include <darray.h>
#include <memory.h>
#include <keyval.h>
#include <malka/ml_states.h>

#ifdef TARGET_IOS
#include <gamecenter.h>
#endif

const int max_levels = 50;
const int solve_bonus = 11;

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

	n = mml_get_child(mml, node, "par_time");
	if(n)
		dest->par_time = mml_getval_uint(mml, n);
	else
		dest->par_time = 0;

	n = mml_get_child(mml, node, "min_time");
	if(n)
		dest->min_time = mml_getval_uint(mml, n);
	else
		dest->min_time = dest->par_time/2;

	n = mml_get_child(mml, node, "par_reactions");
	if(n)
		dest->par_reactions = mml_getval_uint(mml, n);
	else
		dest->par_reactions = 0;

	n = mml_get_child(mml, node, "min_reactions");
	if(n)
		dest->min_reactions = mml_getval_uint(mml, n);
	else
		dest->min_reactions = dest->par_reactions/2;

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

	// Special logic for levels update:
	// unlock 40 & 41 levels if 39 is solved,
	// unlock 40 if 38 is solved

	if(level_is_solved("l39")) {
		keyval_set_bool("ulck_l40", true);
		keyval_set_bool("ulck_l41", true);
	}

	if(level_is_solved("l38")) {
		keyval_set_bool("ulck_l40", true);
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

	mml_free(&ed_mml);
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

static int _is_accessible(int n) {
	if(n+1 <= 10 || n+1 == 50)
		return true;
	if(keyval_get_bool("unlocked", false))
		return true;
	return false;
}

const char* levels_next(const char* current) {
	LevelDef* defs = DARRAY_DATA_PTR(level_defs, LevelDef);
	for(uint i = 0; i < level_defs.size; ++i) {
		if(strcmp(defs[i].name, current) == 0) {
			if(_is_accessible(i))
				return defs[(i+1)%level_defs.size].name;
			else
				return NULL;
		}
	}
	assert(0 && "Level not found");
	return NULL;
}

void levels_close(void) {
	if(level_defs_allocated) {
		darray_free(&level_defs);
		level_defs_allocated = false;
		level_mml_loaded = false;
	}
}

bool level_is_unlocked(const char* name) {
	char key_name[32] = "ulck_";
	assert(strlen(key_name) + strlen(name) < 32);
	strcat(key_name, name);

	return strcmp(name, "l1") == 0 || strcmp(name, "l40") == 0 || keyval_get_bool(key_name, false);
}

bool level_is_unlocked_n(uint n) {
	char key_name[32];
	sprintf(key_name, "ulck_l%u", n);

	return n == 1 || n == 50 || keyval_get_bool(key_name, false);
}

bool level_is_solved(const char* name) {
	char key_name[32] = "slvd_";
	assert(strlen(key_name) + strlen(name) < 32);
	strcat(key_name, name);

	return keyval_get_bool(key_name, false);
}

void level_solve(const char* name, uint reactions, uint time) {
	char key_name[32];

	int levelnum;
	sscanf(name, "l%d", &levelnum);

	// Mark level solved
	sprintf(key_name, "slvd_%s", name);
	keyval_set_bool(key_name, true);

	// Unlock next 2 levels
	for(uint i = 1; i <= 2; ++i) {
		if(levelnum + i <= max_levels) {
			sprintf(key_name, "ulck_l%u", levelnum + i);
			keyval_set_bool(key_name, true);
		}
	}

	// Update time
	sprintf(key_name, "time_%s", name);
	keyval_set_int(key_name, time);

	// Update score
	LevelDef def;
	levels_get(name, &def);
	int score = solve_bonus;
	if(def.par_time)
		score += MAX(0, (int)def.par_time - (int)time);
	if(def.par_reactions)
		score += MAX(0, (int)def.par_reactions - (int)reactions);
	sprintf(key_name, "score_%s", name);
	uint old_score = keyval_get_int(key_name, 0);
	//uint total_score = levels_total_score();
	if(score > old_score) {
		keyval_set_int(key_name, score);

		// Submit score to gamecenter
#ifdef TARGET_IOS
		if(gamecenter_is_active()) {
			GameCenterScore s = {
				.category = "default",
				.context = 42,
				.value = levels_total_score(),
				.player = NULL
			};

			gamecenter_report_score(&s);
		}
//#else
//	total_score = 0;
#endif
	}
}

float level_score(const char* name) {
	char key_name[32] = "score_";
	assert(strlen(key_name) + strlen(name) < 32);

	LevelDef def;
	levels_get(name, &def);
	uint max_score = def.par_time - def.min_time;
	max_score += (def.par_reactions - def.min_reactions) + solve_bonus;
	return clamp(0.0f, 1.0f, (float)keyval_get_int(key_name, 0) / (float)max_score);
}

float level_score_n(uint n) {
	char key_name[32];
	sprintf(key_name, "score_l%u", n);

	assert(n >= 1 && n <= max_levels);
	LevelDef* defs = DARRAY_DATA_PTR(level_defs, LevelDef);
	uint max_score = defs[n-1].par_time - defs[n-1].min_time;
	max_score += defs[n-1].par_reactions - defs[n-1].min_reactions;
	max_score += solve_bonus;

	return clamp(0.0f, 1.0f, (float)keyval_get_int(key_name, 0) / (float)max_score);
}

uint levels_total_score(void) {
	uint score = 0;
	static char level_name[16];
	char key_name[16];
	uint n_perfected_levels = 0;
	uint n_solved_levels = 0;
	uint total_time = 0;
	LevelDef* defs = DARRAY_DATA_PTR(level_defs, LevelDef);
	for(uint i = 1; i <= max_levels; ++i) {
		sprintf(level_name, "l%d", i);
		sprintf(key_name, "score_%s", level_name);
		uint s = keyval_get_int(key_name, 0);
		uint max_score = defs[i].par_time - defs[i].min_time;
		max_score += (defs[i].par_reactions - defs[i].min_reactions) + solve_bonus;
		if(max_score == s)
			n_perfected_levels++;

		sprintf(key_name, "slvd_%s", level_name);
		if(keyval_get_bool(key_name, false)) {
			n_solved_levels++;
			sprintf(key_name, "time_%s", level_name);
			total_time += keyval_get_int(key_name, 1000);
		}
		score += s;
	}

	if(n_solved_levels >= max_levels-1) {
		if(total_time <= 60*60)
			achievements_progress("flash", 1.0f);
	}

	achievements_progress("perfectionist", clamp(0.0f, 1.0f, (float)n_perfected_levels / 15.0f));
	achievements_progress("tour_de_force", clamp(0.0f, 1.0f, (float)n_perfected_levels / (float)(max_levels-1)));

	return score;
}

const char* level_first_unsolved(void) {
	static char level_name[16];
	char key_name[16];
	for(uint i = 1; i <= max_levels; ++i) {
		sprintf(level_name, "l%d", i);

		sprintf(key_name, "slvd_%s", level_name);
		if(!keyval_get_bool(key_name, false)) {
			sprintf(key_name, "ulck_%s", level_name);
			if(i == 1 || keyval_get_bool(key_name, false)) {
				if(_is_accessible(i-1))
					return level_name;
				else {
					malka_states_push("menu");
					return level_name;
				}
			}
		}
	}
	assert(0 && "Unable to determine next level to solve!");
	return "l1";
}

