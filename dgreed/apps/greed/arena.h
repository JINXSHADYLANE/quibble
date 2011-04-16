#ifndef ARENA_H
#define ARENA_H

#include "utils.h"
#include "system.h"
#include "game.h"
#include "ai_precalc.h"

#define MAX_ARENAS_IN_CHAPTER 10 
#define MAX_CHAPTERS 5

#define DENSITY_MAP_WIDTH 30
#define DENSITY_MAP_HEIGHT 20

typedef struct {
	const char* name;
	uint n_arenas;
	const char* arena_file[MAX_ARENAS_IN_CHAPTER];
	const char* arena_name[MAX_ARENAS_IN_CHAPTER];
	uint arena_players[MAX_ARENAS_IN_CHAPTER];
} ChapterDesc;	


typedef struct {
	TexHandle background_img;
	TexHandle walls_img;
	bool no_wall_shadows;

	Vector2 shadow_shift;
	Vector2 spawnpoints[MAX_SHIPS];
	uint n_platforms;
	Vector2* platforms;

	uint n_tris;
	Triangle* collision_tris;
	float* density_map;
	NavMesh nav_mesh;

} ArenaDesc;

extern uint total_arenas;
extern ChapterDesc chapters[MAX_CHAPTERS];
extern ArenaDesc current_arena_desc;

void arenas_init(void);
void arenas_close(void);

void arena_init(void);
void arena_close(void);
void arena_reset(const char* filename, uint n_ships);

void arena_update(float dt);
void arena_draw(void);
void arena_draw_transition(float t);

const char* arena_get_current(void);
const char* arena_get_next(void);

// AI helpers:
uint arena_closest_navpoint(Vector2 pos);
uint arena_platform_navpoint(uint platform);

#endif
